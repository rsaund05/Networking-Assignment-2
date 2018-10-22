CIS 3210 Networking Assignment 2

Name: 
    Corey Zimmer 0924956
    Rob Saunders 0891696

Compilation: 
    make

Execution: [] means optional
    bin/server portnum
    bin/client IP:port filename [buffer length]

Script to spawn 5 clients in parallel:
    make spawn

NOTE: Files that are downloaded to the disk are stored in the 'files' directory

Known issues:
    - Server can accept a total of 10 connections
    - Duplicate filenames running at the same time causes undefined 
    - If a directory called "files" does not exist, the program will seg fault
    - Files to be transferred must be in the same directory as the client, and only use the actual filename (no paths) e.g "wonderland.txt" NOT "assets/wonderland.txt" and NOT "./wonderland.txt"
    - If server is terminated before downloads finished, zombie threads remain
    - make spawn is hard-coded to port 12079