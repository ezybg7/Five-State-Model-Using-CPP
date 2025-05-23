#include "process.h"
#include "ioModule.h"
#include "processMgmt.h"

#include <chrono> // for sleep
#include <thread> // for sleep

int main(int argc, char* argv[])
{
    // single thread processor
    // it's either processing something or it's not
    //bool processorAvailable = true;

    // vector of processes, processes will appear here when they are created by
    // the ProcessMgmt object (in other words, automatically at the appropriate time)
    list<Process> processList;
    
    // this will orchestrate process creation in our system, it will add processes to 
    // processList when they are created and ready to be run/managed
    ProcessManagement processMgmt(processList);

    // this is where interrupts will appear when the ioModule detects that an IO operation is complete
    list<IOInterrupt> interrupts;   

    // this manages io operations and will raise interrupts to signal io completion
    IOModule ioModule(interrupts);  

    // Do not touch
    long time = 1;
    long sleepDuration = 50;
    string file;
    stringstream ss;
    enum stepActionEnum {noAct, admitNewProc, handleInterrupt, beginRun, continueRun, ioRequest, complete} stepAction;

    // Do not touch
    switch(argc)
    {
        case 1:
            file = "./procList.txt";  // default input file
            break;
        case 2:
            file = argv[1];         // file given from command line
            break;
        case 3:
            file = argv[1];         // file given
            ss.str(argv[2]);        // sleep duration given
            ss >> sleepDuration;
            break;
        default:
            cerr << "incorrect number of command line arguments" << endl;
            cout << "usage: " << argv[0] << " [file] [sleepDuration]" << endl;
            return 1;
            break;
    }

    processMgmt.readProcessFile(file);


    time = 0;

    list<Process>::iterator here = processList.begin();
    list<Process> readyList, blockedList;
    bool running = false, cont = false, allFinished = false, processorAvailable = true;

    //keep running the loop until all processes have been added and have run to completion
    while(!allFinished)
    {
        //Update our current time step
        ++time;

        //let new processes in if there are any
        processMgmt.activateProcesses(time);

        //update the status for any active IO requests
        ioModule.ioProcessing(time);

        //If the processor is tied up running a process, then continue running it until it is done or blocks
        //   note: be sure to check for things that should happen as the process continues to run (io, completion...)
        //If the processor is free then you can choose the appropriate action to take, the choices (in order of precedence) are:
        // - admit a new process if one is ready (i.e., take a 'newArrival' process and put them in the 'ready' state)
        // - address an interrupt if there are any pending (i.e., update the state of a blocked process whose IO operation is complete)
        // - start processing a ready process if there are any ready
        bool admitting = false, finished = false, ioReqBlock = false, intrptDone = false;

        if(!processorAvailable){
          if(running || cont){
            cont = true;
            running = false;
            here->processorTime++;
            for(list<IOEvent>::iterator ite = here->ioEvents.begin(); ite != here->ioEvents.end(); ++ite){
              if(here->processorTime == ite->time){//encountered io event for current process
                here->state = blocked; 
                blockedList.push_back(*here);
                ioModule.submitIORequest(time, *ite, *here);
                ioReqBlock = true;
                processorAvailable = true;
                cont = false;
              } 
            }
            if(here->processorTime == here->reqProcessorTime){//process done
              finished = true;
              processorAvailable = true;
              here->state = done;
              here->doneTime = time;
            }
          }
        }else{
          //set newly arriving processes to ready status one at a time and adding to ready list
          for(list<Process>::iterator here2 = processList.begin(); here2 != processList.end(); ++here2){
            if(here2->state == newArrival){
                here2->state = ready;
                readyList.push_back(*here2);
                admitting = true;
                break;
            }
          }
          //address interrupt if any pending
          if(!interrupts.empty() && !admitting) //!interrupts.empty() for when only one process running !admitting so process don't both happen during same time step
            for(list<Process>::iterator here2 = processList.begin(); here2 != processList.end(); ++here2)
              if(here2->state == blocked && here2->id == interrupts.front().procID){
                interrupts.pop_front();
                for(list<Process>::iterator here3 = blockedList.begin(); here3 != blockedList.end(); ++here3){
                  if(here3->id == here2->id){
                    readyList.push_back(*here3);
                    blockedList.erase(here3);
                    break;
                  }
                }
                intrptDone = true;
                here2->state = ready;
                break;
              }
          
          if(!admitting && !intrptDone){//FIFO
            here = processList.begin();
            bool madeReady = false;
            while(!madeReady && !readyList.empty()){
              if(here->id == readyList.front().id){
                here->state = processing;
                readyList.pop_front();
                running = true;
                processorAvailable = false;
                madeReady = true;
              } else
                ++here;
            }
          }
        }

        long unsigned int doneCounter = 0;//if list isn't empty check if process not on time 1
        if(!processList.empty()){
          for(list<Process>::iterator here2 = processList.begin(); here2 != processList.end(); ++here2){
            if(here2->state == done)
              doneCounter++;
          }
          if(doneCounter == processList.size())
          allFinished = true;
        }
          
  
        //init the stepAction, update below
        stepAction = noAct;

        
        //TODO add in the code to take an appropriate action for this time step!
        //you should set the action variable based on what you do this time step. you can just copy and paste the lines below and uncomment them, if you want.
        //stepAction = continueRun;  //runnning process is still running
        //stepAction = ioRequest;  //running process issued an io request
        //stepAction = complete;   //running process is finished
        //stepAction = admitNewProc;   //admit a new process into 'ready'
        //stepAction = handleInterrupt;   //handle an interrupt
        //stepAction = beginRun;   //start running a process

        //   <your code here> 

        if(admitting)
          stepAction = admitNewProc;
        else if(finished)
          stepAction = complete;
        else if(ioReqBlock)
          stepAction = ioRequest;
        else if(intrptDone)
          stepAction = handleInterrupt;
        else if(running)
          stepAction = beginRun;
        else if(cont)
          stepAction = continueRun;

        // Leave the below alone (at least for final submission, we are counting on the output being in expected format)
        cout << setw(5) << time << "\t"; 
        
        switch(stepAction)
        {
            case admitNewProc:
              cout << "[  admit]\t";
              break;
            case handleInterrupt:
              cout << "[ inrtpt]\t";
              break;
            case beginRun:
              cout << "[  begin]\t";
              break;
            case continueRun:
              cout << "[contRun]\t";
              break;
            case ioRequest:
              cout << "[  ioReq]\t";
              break;
            case complete:
              cout << "[ finish]\t";
              break;
            case noAct:
              cout << "[*noAct*]\t";
              break;
        }

        // You may wish to use a second vector of processes (you don't need to, but you can)
        printProcessStates(processList); // change processList to another vector of processes if desired

        this_thread::sleep_for(chrono::milliseconds(sleepDuration));
    }

    return 0;
}
