# Simple-HTTP-1.0-Server
Responds to GET Requests

## Introduction
A basic HTTP server that responds correctly to a limited set of GET requests. The server returns valid **response headers** for a range of files and paths. The content it can serve is HTML (.html), JPEG (.jpg), CSS (.css) and JavaScript (.js). The server implements HTTP 1.0 and handles multiple incoming requests with **multithreading**.

## Usage

Run the following on the command line:

      
      ./server [port number] [path to web root directory]
      
You will then be able to send get requests to the server using *wget* or similar

## Task
COMP30023: Computer Systems: Assignment 1
