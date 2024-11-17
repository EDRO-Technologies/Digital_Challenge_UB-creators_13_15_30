# DNSServer

C++ realization of DNS emulator. Developed for SurSU Digital Challenge competitions.

## Team Information

**UB creators** team.

## Dependencies

- Boost library (asio for sockets)
- yaml-cpp for configuration purposes

## demonstration

Логи визуализируются как таблица используя html/css/js <br>

![image](https://github.com/user-attachments/assets/ba8be8bb-97c1-4adf-8305-cffbfb38ea24)
![image copy](https://github.com/user-attachments/assets/89e13a94-9064-4c00-a709-e81b1f047229)
![image copy 2](https://github.com/user-attachments/assets/ea4ecc72-5ae7-4008-bc18-2fef4a3a347a)
![image copy 3](https://github.com/user-attachments/assets/279e9a20-ee2e-4c3a-94ff-f24953fe1c99)

## Install and Startup guide

#### Install

    mkdir build && cd build
    cmake ..
    make

## Configure

Configuration file has to be in yaml format and has to contains folowing fields:

- `log_filename` - Path to log file and base name;
- `logfile_size` - Maximum log file size (in kilobytes);
- `port` - Port number, program to be started on;
- `dns_server` - Preferred DNS server.

It may looks like this:

    log_filename: "application.log"
    logfile_size: 512
    port: 8080
    dns_server: "8.8.8.8"

#### Startup

    ./DNSServer <path_to_config>

## Todo

Empty, finally... ;)

I know, it needs to get logging system, specialized for such type of software, but... I'm done, really...

P.S. And maybe it needs some good documentation, but I'm not shure, that I probably can write really good documentation after almost 16 hours of coding... Sorry, tutors, judges, observers, and every other, who reads this repository.
