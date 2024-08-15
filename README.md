# FPGA-Based TSP Solver on Digilent Zybo Z7-10

This project implements a hardware-based solution to the Travelling Salesperson Problem (TSP) using a Xilinx Zynq-7000 FPGA on a Digilent Zybo Z7-10 board. The solution leverages the parallelism of the FPGA, along with the real-time capabilities of FreeRTOS, to efficiently explore the solution space of the TSP, achieving a remarkable performance of solving 13 cities in under 3 seconds.

## Problem Overview

The Travelling Salesperson Problem (TSP) is a classic problem in computer science where a traveler needs to visit a set of cities exactly once and return to the starting point. The objective is to find the optimal order of cities that minimizes the total travel distance. The problem is known to be NP-hard, meaning that no efficient algorithm is known to solve all instances quickly.

This project explores the use of FPGA hardware to solve the TSP by implementing a brute-force (and Branch and Bound) approach that takes advantage of the parallel computing capabilities of the FPGA, managed by the FreeRTOS operating system for efficient task scheduling and management.

## Project Features

- **FPGA Platform:** Xilinx Zynq-7000 on Digilent Zybo Z7-10
- **Solution Approach:** Brute-force search using custom hardware IP cores (alternative Branch and Bound is also provided)
- **Real-Time OS**: FreeRTOS for task management and scheduling
- **Performance:** Able to solve TSP for 13 cities in under 3 seconds
- **Communication:** Interacts with a network server to request and solve TSP scenarios
- **Protocol:** Uses a custom UDP-based protocol to communicate with the server

## Resource Utilization

The implementation efficiently utilizes the FPGA resources as detailed below:

| Resource | Utilization | Available | Utilization (%) |
|----------|-------------|-----------|-----------------|
| LUT      | 10,804      | 17,600    | 61.39%          |
| LUTRAM   | 3,008       | 6,000     | 50.13%          |
| FF       | 8,503       | 35,200    | 24.16%          |
| BRAM     | 0.5         | 60        | 0.83%           |
| DSP      | 80          | 80        | 100%            |
| IO       | 15          | 100       | 15%             |
| BUFG     | 1           | 32        | 3.13%           |

## Scenario Communication

The project involves communication with a scenario server, which provides the TSP problems to solve. The communication is based on a custom protocol using UDP packets, where the FPGA requests a scenario, solves it, and sends the solution back to the server.

**Request Packet Format:**
- Message ID: 1 byte (0x01)
- Number of Cities: 1 byte (4 to 20 inclusive)
- Scenario ID: 4 bytes (Big-endian integer)

**Response Packet Format:**
- Message ID: 1 byte (0x02)
- Number of Cities: 1 byte (Matches request)
- Scenario ID: 4 bytes (Matches request)
- Adjacency Matrix: N x N matrix (where N is the number of cities)

**Solution Packet Format:**
- Message ID: 1 byte (0x03)
- Number of Cities: 1 byte (4 to 20 inclusive)
- Scenario ID: 4 bytes (Big-endian integer)
- Shortest Path: 4 bytes (Big-endian integer)
