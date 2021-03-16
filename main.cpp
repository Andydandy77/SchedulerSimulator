/**
 * Dave Anderson and Denis Rajic
 * CPSC 5041, Seattle University
 *
 * This program provides the user to read in an input file of processes, and choose between
 * four scheduling algorithms: SJF, SRTF, NP, PP. The program will schedule the processes,
 * and print out when each process starts, and statistics about CPU usage.
 */

#include <iostream>
#include <iomanip>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <queue>
#include <cmath>

using namespace std;

/**
 * Holds information on a process
 */
struct process {
    int ID;
    int arrivalTime;
    int burstTime;
    int executionTimeLeft;
    int priority;
    int termination;
};

vector<process *> readFile(const string &filename);

void nonPreemptive(vector<process *> &processes, const string &algorithm);

void preemptive(vector<process *> &processes, const string &algorithm);

void sortReadyQueue(queue<process *> &readyQueue);

void sortNPQueue(queue<process *> &readyQueue);

void sortByArrival(vector<process *> &processes);

void printStatistics(int time, int idleTime,
                     int totalTurnaround, int totalBurstTime,
                     int worstCaseWaitTime, int processesSize);

int main(int argc, char *argv[]) {
    string filename = argv[1];
    string algorithm = argv[2];

    // Fill vector with all processes
    vector<process *> processes = readFile(filename);

    // Initially sort processes by arrival time
    sortByArrival(processes);

    if (algorithm == "SJF" || algorithm == "NP") {
        nonPreemptive(processes, algorithm);
    } else if (algorithm == "SRTF" || algorithm == "PP") {
        preemptive(processes, algorithm);
    } else {
        cout << "Error: Invalid sorting algorithm" << endl;
    }

    for (process *p : processes) {
        delete p;
    }

    return 0;
}

/**
 * Reads file and returns a vector of processes
 * @param filename
 * @return
 */
vector<process *> readFile(const string &filename) {
    vector<process *> processes;

    ifstream inputFile;
    inputFile.open(filename);
    string line;


    if (inputFile.is_open()) {
        while (getline(inputFile, line)) {
            auto *p = new process{};
            stringstream ss(line);

            getline(ss, line, ' ');
            p->ID = stoi(line);

            getline(ss, line, ' ');
            p->arrivalTime = stoi(line);

            getline(ss, line, ' ');
            p->burstTime = stoi(line);

            getline(ss, line, ' ');
            p->priority = stoi(line);

            p->executionTimeLeft = p->burstTime;

            processes.push_back(p);
        }
    }

    return processes;
}

/**
 * Shortest Job First Scheduling Process
 *
 * @param processes List of processes that will be executed
 * @param algorithm User argument for sorting by burst time or priority
 */
void nonPreemptive(vector<process *> &processes, const string &algorithm) {
    queue<process *> yetToArrive;
    queue<process *> readyQueue;

    // Move processes into a queue
    for (process *p: processes) {
        yetToArrive.push(p);
    }


    int time = 0;       // Tracks current time
    int idleTime = 0;   // Tracks idle time
    int totalTurnaround = 0;    // Tracks total turnaround time
    int totalBurstTime = 0;     // Tracks total burst time
    int worstCaseWaitTime = 0;  // Tracks worst case waiting time

    // Loop through all processes until queue is empty
    while (!yetToArrive.empty()) {

        // Loop through each process in the yetToArrive queue and push the
        // items that have arrived to readyQueue
        while (!yetToArrive.empty() &&
               yetToArrive.front()->arrivalTime <= time) {
            process *p = yetToArrive.front();
            // Move process to ready queue
            readyQueue.push(p);
            // Remove process from yet to arrive queue
            yetToArrive.pop();
        }

        // Loop through processes that have arrived in readyQueue
        if (readyQueue.empty()) {
            // If readyQueue is empty, add to idleTime
            idleTime += yetToArrive.front()->arrivalTime - time;
            cout << "Time " << time << " Idle" << endl;
            // Move current time to next items arrival time
            time = yetToArrive.front()->arrivalTime;
        } else {

            // Sorts readyQueue if there are multiple processes in queue by
            // specified algorithm (burst time or priority)
            if (readyQueue.size() > 1) {
                if (algorithm == "SJF") {
                    sortReadyQueue(readyQueue);
                } else {
                    sortNPQueue(readyQueue);
                }
            }


            while (!readyQueue.empty()) {
                // Current process
                process *currentProcess = readyQueue.front();
                currentProcess->termination = time + currentProcess->burstTime;
                int turnaround = currentProcess->termination - currentProcess->arrivalTime;
                int waitTime = turnaround - currentProcess->burstTime;

                // Update worst case wait time
                if (waitTime > worstCaseWaitTime) {
                    worstCaseWaitTime = waitTime;
                }

                // Update total turnaround time
                totalTurnaround += turnaround;
                // Update total burst time
                totalBurstTime += currentProcess->burstTime;

                // Remove process from ready queue
                readyQueue.pop();

                // Print status to screen
                cout << "Time " << time << " Process " << currentProcess->ID
                     << endl;

                // Increment time
                time += currentProcess->burstTime;

                // Check if there are other processes that have arrived
                while (!yetToArrive.empty() &&
                       yetToArrive.front()->arrivalTime <= time) {
                    process *p = yetToArrive.front();
                    readyQueue.push(p);
                    yetToArrive.pop();

                    if (algorithm == "SJF") {
                        sortReadyQueue(readyQueue);
                    } else {
                        sortNPQueue(readyQueue);
                    }
                }
            }
        }
    }

    printStatistics(time, idleTime, totalTurnaround, totalBurstTime, worstCaseWaitTime, processes.size());

}

/**
 * Shortest Remaining Time First Scheduling Process
 *
 * @param processes List of processes that will be executed
 * @param algorithm User argument for sorting by burst time or priority
 */
void preemptive(vector<process *> &processes, const string &algorithm) {
    queue<process *> yetToArrive;
    queue<process *> readyQueue;

    // Move processes into a queue
    for (process *p: processes) {
        yetToArrive.push(p);
    }


    int time = 0;       // Tracks current time
    int idleTime = 0;   // Tracks idle time
    int totalTurnaround = 0;    // Tracks total turnaround time
    int totalBurstTime = 0;     // Tracks total burst time
    int worstCaseWaitTime = 0;  // Tracks worst case waiting time

    // Loop through all processes until queue is empty
    while (!yetToArrive.empty()) {

        // Loop through each process in the yetToArrive queue and push the
        // items that have arrived to readyQueue
        while (!yetToArrive.empty() &&
               yetToArrive.front()->arrivalTime <= time) {
            process *p = yetToArrive.front();
            // Move process to ready queue
            readyQueue.push(p);
            // Remove process from yet to arrive queue
            yetToArrive.pop();
        }

        // Loop through processes that have arrived in readyQueue
        if (readyQueue.empty()) {
            // If readyQueue is empty, add to idleTime
            idleTime += yetToArrive.front()->arrivalTime - time;
            cout << "Time " << time << " Idle" << endl;
            // Move current time to next items arrival time
            time = yetToArrive.front()->arrivalTime;
        } else {

            // Sorts readyQueue if there are multiple processes in queue by
            // specified algorithm (burst time or priority)
            if (readyQueue.size() > 1) {
                if (algorithm == "SRTF") {
                    sortReadyQueue(readyQueue);
                } else {
                    sortNPQueue(readyQueue);
                }
            }

            // bool that is true when a different process is being started or resumed
            bool contextSwitch = true;

            while (!readyQueue.empty()) {
                // Current process
                process *currentProcess = readyQueue.front();

                while (currentProcess->executionTimeLeft > 0) {
                    // if the process switched from the previous iteration
                    if (contextSwitch) {
                        cout << "Time " << time << " Process " << currentProcess->ID << endl;
                        contextSwitch = false;
                    }
                    time++;
                    currentProcess->executionTimeLeft--; // decrement execution time
                    // if execution completes, remove it from the queue, and update time statistics
                    if (currentProcess->executionTimeLeft == 0) {
                        readyQueue.pop();
                        currentProcess->termination = time;
                        int turnaround = currentProcess->termination - currentProcess->arrivalTime;
                        int waitTime = turnaround - currentProcess->burstTime;
                        if (waitTime > worstCaseWaitTime) {
                            worstCaseWaitTime = waitTime;
                        }
                        totalBurstTime += currentProcess->burstTime;
                        totalTurnaround += turnaround;
                    }

                    // bool to see if newProcesses have entered the readyQueue
                    bool newProcesses;

                    // loops through the processes yet to Arrive and adds them to the readyQueue if they have arrived
                    while (!yetToArrive.empty() && time >= yetToArrive.front()->arrivalTime) {
                        readyQueue.push(yetToArrive.front());
                        yetToArrive.pop();
                        newProcesses = true;
                    }

                    // if there are more processes ready, sort them according to burst time or priority
                    if (!readyQueue.empty()) {
                        if (newProcesses) {
                            (algorithm == "SRTF") ? sortReadyQueue(readyQueue) : sortNPQueue(readyQueue);
                            if (currentProcess != readyQueue.front()) {
                                contextSwitch = true;
                            }
                            // set the current process to the ready processes with the shortest burst time
                            currentProcess = readyQueue.front();
                        }
                    }
                }
            }
        }
    }

    printStatistics(time, idleTime, totalTurnaround, totalBurstTime, worstCaseWaitTime, processes.size());

}

/**
 * Sorts readyQueue by lowest burst time left
 * @param readyQueue
 */
void sortReadyQueue(queue<process *> &readyQueue) {
    vector<process *> v;

    // Move processes to vector to allow sorting
    while (!readyQueue.empty()) {
        process *p = readyQueue.front();
        readyQueue.pop();
        v.push_back(p);
    }

    // Sort processes by burst time
    for (int i = 0; i < v.size(); i++) {
        for (int j = i + 1; j < v.size(); j++) {
            if (v.at(i)->executionTimeLeft > v.at(j)->executionTimeLeft ||
                    (v.at(i)->executionTimeLeft == v.at(j)->executionTimeLeft && v.at(i)->ID > v.at(j)->ID))
                swap(v.at(i), v.at(j));
        }
    }


    // Move sorted processes back to queue
    for (auto &i : v) {
        readyQueue.push(i);
    }
}

/**
 * Sorts queue by priority
 *
 * @param readyQueue queue to be sorted
 */
void sortNPQueue(queue<process *> &readyQueue) {
    vector<process *> v;

    // Move processes to vector to allow sorting
    while (!readyQueue.empty()) {
        process *p = readyQueue.front();
        readyQueue.pop();
        v.push_back(p);
    }

    // Sort processes by priority
    for (int i = 0; i < v.size(); i++) {
        for (int j = i + 1; j < v.size(); j++) {
            if (v.at(i)->priority > v.at(j)->priority) {
                swap(v.at(i), v.at(j));
            }
        }
    }

    // Move sorted processes back to queue
    for (auto &i : v) {
        readyQueue.push(i);
    }
}

/**
 * Sorts processes by arrival time
 *
 * @param processes to sort by arrival time
 */
void sortByArrival(vector<process *> &processes) {
    // Sort processes by arrivalTime
    for (int i = 0; i < processes.size(); i++) {
        for (int j = i + 1; j < processes.size(); j++) {
            if (processes.at(i)->arrivalTime > processes.at(j)->arrivalTime
                    ) {
                swap(processes.at(i), processes.at(j));
            }

        }
    }
}

/**
 * Print Timing Statistics
 * @param time finishing time
 * @param idleTime time with no processes running
 * @param totalTurnaround
 * @param totalBurstTime
 * @param worstCaseWaitTime
 * @param processesSize number of processes
 */
void printStatistics(int time, int idleTime,
                     int totalTurnaround, int totalBurstTime,
                     int worstCaseWaitTime, int processesSize) {

    // Print stats on cpu utilization, avg wait time, worst-case wait time
    cout << "CPU Utilization: " << round(100 * (time - idleTime) / (double)
            time) << "%" << endl;
    double avgWaitTime = (totalTurnaround - totalBurstTime) / (double)
            processesSize;
    printf("Average waiting time: %.2f\n", avgWaitTime);
    cout << "Worst-case waiting time: " << worstCaseWaitTime << endl;

}
