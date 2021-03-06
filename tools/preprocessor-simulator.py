#!/usr/bin/env python
'''
preprocessor-simulator.py

Simulate a receiver with built-in preprocessor. This allow testing of the
light controller functionality without hooking up a RC system.

A web browser is used for the user interface

Author:         Werner Lane
E-mail:         laneboysrc@gmail.com
'''

from __future__ import print_function

import sys
import os
import argparse
import serial
import time
import threading

try:
    from BaseHTTPServer import BaseHTTPRequestHandler, HTTPServer
except ImportError:
    from http.server import BaseHTTPRequestHandler, HTTPServer

try:
    from urlparse import parse_qs
except ImportError:
    from urllib.parse import parse_qs


SLAVE_MAGIC_BYTE = 0x87
HTML_FILE = "preprocessor-simulator.html"


class QuietBaseHTTPRequestHandler(BaseHTTPRequestHandler):
    def log_request(self, code, message=None):
        ''' Supress logging of HTTP requests '''
        pass


def parse_commandline():
    ''' Simulate a receiver with built-in preprocessor '''
    parser = argparse.ArgumentParser(
        description="Simulate a receiver with built-in preprocessor.")

    parser.add_argument("-b", "--baudrate", type=int, default=38400,
        help='Baudrate to use. Default is 38400.')

    parser.add_argument("-p", "--port", type=int, default=1234,
        help='HTTP port for the web UI. Default is localhost:1234.')

    parser.add_argument("tty", nargs="?", default="/dev/ttyUSB0",
        help="serial port to use. ")

    return parser.parse_args()


class CustomHTTPRequestHandler(QuietBaseHTTPRequestHandler):
    ''' Request handler that implements our simple web based API '''

    def do_GET(self):
        ''' GET request handler '''
        self.send_response(200)
        self.send_header('Content-type', 'text/html')
        self.end_headers()

        html_path = os.path.join(
            os.path.dirname(os.path.abspath(sys.argv[0])),
            HTML_FILE)

        with open(html_path, "r") as html_file:
            self.wfile.write(html_file.read().encode("UTF-8"))
        return

    def do_POST(self):
        ''' POST request handler '''
        query = self.rfile.read(
            int(self.headers['Content-Length'])).decode("UTF-8")
        try:
            query = parse_qs(query, strict_parsing=True)

        except ValueError:
            self.send_response(400)
            self.send_header('Content-type', 'text/html')
            self.end_headers()
            self.wfile.write("Bad querystring")

        else:
            response, content = self.server.preprocessor.api(query)
            self.send_response(response)
            self.send_header('Content-type', 'text/html')
            self.end_headers()
            self.wfile.write(content.encode('UTF-8'))
        return


class PreprocessorApp(object):
    ''' Simulate a RC receiver with preprocessor '''

    def __init__(self):
        self.args = parse_commandline()
        self.receiver = {
            'ST': 0, 'TH': 0, 'CH3': 0, 'STARTUP_MODE': 1, 'PING' : 0}
        self.read_thread = None
        self.write_thread = None
        self.done = False

        try:
            self.uart = serial.Serial(self.args.tty, self.args.baudrate)
        except serial.SerialException as error:
            print("Unable to open port %s: %s" % (self.args.tty, error))
            sys.exit(1)

        print("Simulating on {uart} at {baudrate} baud.".format(
            uart=self.uart.port, baudrate=self.uart.baudrate))

    def api(self, query):
        ''' Web api handler '''
        for key, value in query.items():
            if key in self.receiver:
                self.receiver[key] = int(value[0])
            else:
                return 400, "Bad request"
        return 200, "OK"

    def run(self):
        ''' Send the test patterns to the TLC5940 based slave '''

        def reader(app):
            ''' Background thread performing the UART read '''
            time_of_last_line = start_time = time.time()
            print("     TOTAL  DIFFERENCE  RESPONSE")
            print("----------  ----------  --------")

            while not app.done:
                app.uart.timeout = 0.1
                data = app.uart.readline()
                if len(data):
                    current_time = time.time()
                    time_difference = current_time - time_of_last_line
                    elapsed_time = current_time - start_time

                    print("%10.3f  %10.3f  %s" % (elapsed_time,
                        time_difference,
                        data.decode('ascii', errors='replace')),
                        end='')

                    time_of_last_line = current_time

        def writer(app):
            ''' Background thread performing the UART transmission '''
            while not app.done:
                steering = app.receiver['ST']
                if steering < 0:
                    steering = 256 + steering

                throttle = app.receiver['TH']
                if throttle < 0:
                    throttle = 256 + throttle

                last_byte = 0
                if app.receiver['CH3']:
                    last_byte += 0x01
                if app.receiver['STARTUP_MODE']:
                    last_byte += 0x10

                data = bytearray(
                    [SLAVE_MAGIC_BYTE, steering, throttle, last_byte])
                app.uart.write(data)
                app.uart.flush()

                time.sleep(0.02)

        server = HTTPServer(('', self.args.port), CustomHTTPRequestHandler)
        server.preprocessor = self

        print("Please call up the user interface on localhost:{port}".format(
            port=self.args.port))

        self.read_thread = threading.Thread(target=reader, args=([self]))
        self.write_thread = threading.Thread(target=writer, args=([self]))
        self.read_thread.start()
        self.write_thread.start()
        server.serve_forever()

    def shutdown(self):
        ''' Shut down the application, wait for the uart thread to finish '''
        self.done = True
        if not self.write_thread is None:
            self.write_thread.join()
        if not self.read_thread is None:
            self.read_thread.join()


def main():
    ''' Program start '''
    app = PreprocessorApp()
    try:
        app.run()
    except KeyboardInterrupt:
        print("")
        app.shutdown()
        sys.exit(0)


if __name__ == '__main__':
    main()
