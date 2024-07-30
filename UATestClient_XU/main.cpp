#include "uaplatformlayer.h"
#include "sampleclient.h"
#include "shutdown.h"
#include "configuration.h"
#include "uathread.h"
#include <cmath>
#include "uavariant.h"
#include <chrono>
#include <iostream>
#include <fstream>
#include <thread>
#include <stdio.h>
#include <vector>


int stop_thread = 0;
int anzahl_methodcalls = 0;

void printProgressBar(int current, int total, int barWidth = 50) {
    float progress = (float)current / total;
    int pos = barWidth * progress;

    std::cerr << "[";
    for (int i = 0; i < barWidth; ++i) {
        if (i < pos) std::cerr << "=";
        else if (i == pos) std::cerr << ">";
        else std::cerr << " ";
    }
    std::cerr << "] " << int(progress * 100.0) << " %\r";
    std::cerr.flush();
}

static void monitorVariables(SampleClient* pMyClient)
{
	UaStatus status;
    std::vector<long long int> times;
    std::ofstream outfile("testcase_thread_received_signals.txt");
    int i = 0;

	status = pMyClient->read();
	UaVariant oldValue(pMyClient->getReadValue());
    auto start = std::chrono::high_resolution_clock::now();

    while (stop_thread == 0)
	{
		status = pMyClient->read();
        

        if (oldValue != pMyClient->getReadValue())
        {
            auto end = std::chrono::high_resolution_clock::now();
            /*if ((i % 10) == 0)
            {
                fprintf(stderr, "\n%d/%d Operations completed successfully!\n", i, ANZ_METHODCALLS);
            }*/
            printProgressBar(i, anzahl_methodcalls);
            auto int_s = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
            times.push_back(int_s.count());
            outfile << "[" << (i + 1) << "]: " << int_s.count() << ":" << atof(pMyClient->getReadValue().toString().toUtf8()) << '\n';

            oldValue = pMyClient->getReadValue();
            i++;
            auto start = std::chrono::high_resolution_clock::now();
        }

	}

    {
        long long int summe = 0;
        for (const auto x : times)
        {
            summe += x;
        }


        outfile << "#\n# Expired test time: [us]" << summe;
        outfile << "\n# Expired test time: [ms]" << summe / 1000.0;
        outfile << "\n# Expired test time: [s]" << summe / 1000000.0;
        outfile << "\n# Expired test time: [min]" << summe / (1000000 * 60.0) << "\n#";

        outfile << "\n# Average time: [us] " << ((float)summe / anzahl_methodcalls);
        outfile << "\n# Average time: [ms] " << ((float)summe / anzahl_methodcalls) / 1000.0;
    }

    outfile.close();
}

/*============================================================================
 * main
 *===========================================================================*/
#ifdef _WIN32_WCE
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPWSTR, int)
#else
int main(int argc, char* argv[])
#endif
{
    if (argc < 4) {
        if (argc > 1 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--h") == 0 || strcmp(argv[1], "-help") == 0)) {
            printf("---------------------------------------------------\n");
            printf("------ Help message of UATestClient_XU usage! -----\n\n");
            printf("Usage: UATestClient_XU.exe KKS CLIENT_ID QUANTITY\n\n");
            printf("KKS = String (with single spaces! Example: AHP 712MP XQ01).\n");
            printf("CLIENT_ID = Identification for client.\n");
            printf("QUANTITY = Number of test repetitions.\n\n");
            printf("---------------------------------------------------\n");

            return 1;
        }
        else {
            std::cerr << "Usage error! -h for help.\n" << std::endl;
        }
    }

    
    std::stringstream kks_stream;
    kks_stream << argv[1] << " " << argv[2] << "               " << argv[3] << "     ";
    //std::cout << kks_stream.str() << std::endl;

    int client_id = std::stoi(argv[4]);

    anzahl_methodcalls = std::stoi(argv[5]);

    if (anzahl_methodcalls <= 0)
    {
        std::cerr << "Please enter a positive number for quantity!\n";
    }

    UaStatus status;
    char mode = ' ';


    // Initialize the UA Stack platform layer
    UaPlatformLayer::init();

    // Create instance of SampleClient
    SampleClient* pMyClient = new SampleClient();

    // get current path to build configuration file name
    UaString sConfigFile(getAppPath());
    sConfigFile += "/testclient_config.ini";

    // Create configuration object and load configuration
    Configuration* pMyConfiguration = new Configuration();
    status = pMyConfiguration->loadConfiguration(sConfigFile);

    std::srand((unsigned int)std::time(NULL));

    // Loading configuration succeeded
    if (status.isGood())
    {
        // set configuration
        pMyClient->setConfiguration(pMyConfiguration);

        // Connect to OPC UA Server
        status = pMyClient->connect();
    }
    else
    {
        delete pMyConfiguration;
        pMyConfiguration = NULL;
    }

    RegisterSignalHandler();

    // Connect succeeded
    if (status.isGood())
    {
        // Get the test-mode:
        //std::cerr << "Enter Testcase (1=Thread, 2=Roundloop, 3=OnlyRead): ";
        //std::cin >> mode;
        mode = '2';

        if (mode == '1')
        {
            float* values = new float[anzahl_methodcalls] { 0 };
            std::vector<long long int> times;
            std::thread t1(monitorVariables, pMyClient);
            UaNodeId objectnodeid = UaNodeId(kks_stream.str().c_str(), 2);
            kks_stream << ".writeSignal";
            UaNodeId methodnodeid = UaNodeId(kks_stream.str().c_str(), 2);
            times.push_back(0);
            float val = 0.98f;
            float step = 0.02f;
            bool upwards = true;

            for (int i = 0; i < anzahl_methodcalls; i++)
            {
                auto start = std::chrono::high_resolution_clock::now();
                /*float val = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
                val = val + (rand() % 10);*/


                val += step;

                values[i] = val;

                status = pMyClient->callMethodWithValue(objectnodeid, methodnodeid, values[i]);

                UaThread::msleep(700);
                auto end = std::chrono::high_resolution_clock::now();
                auto int_s = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
                times.push_back(int_s.count());
            }

            UaThread::msleep(2000);
            stop_thread = 1;

            t1.join();

            std::ofstream outfile("testcase_thread_sent_signals.txt");
            for (int i = 0; i < anzahl_methodcalls; i++)
            {
                outfile << times[i] << ":";
                outfile << values[i] << '\n';
            }
                
            outfile.close();


            // Wait for user command.
            fprintf(stderr,"\nPress Enter to disconnect\n");
            getchar();


            // Disconnect from OPC UA Server
            status = pMyClient->disconnect();

            /*int retval = system("testcase_thread\\testcase_thread.py");
            if (retval != 0)
                fprintf(stderr, "\nError opening testcase_thread.py\n");*/
        }
        else if (mode == '2')
        {
            long long int* times = new long long int[anzahl_methodcalls] {};


            std::stringstream sstr;
            sstr << "testcase_roundloop/testcase_roundloop_" << client_id << ".txt";
            std::ofstream outfile(sstr.str().c_str());

            // Überprüfen, ob die Datei erfolgreich geöffnet wurde
            if (!outfile.is_open()) {
                std::cerr << "Fehler: Die Datei konnte nicht geöffnet werden: " << sstr.str() << std::endl;
                return 1; 
            }

            std::cout << "This is Client " << client_id << " with signal " << kks_stream.str() << ".\n";

            std::time_t timenow = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

            // Build header of output file----------------------------
            outfile << "#\n# [Date/Time]   " << std::ctime(&timenow);
            outfile << "#\n# ---- Description ---- \n#\n# ";
            outfile << anzahl_methodcalls << " signals are written.\n";
            outfile << "# After each write operation, the time is measured until the signal change is visible on the server.\n";
            outfile << "# At the end, an average value is calculated which defines the average write duration of a signal.\n#\n# ---------------------\n#\n";
            outfile << "# Time is formatted in microseconds [us]\n#\n";
            // -------------------------------------------------------


            std::string read_id = kks_stream.str();
            read_id.append(".Value");
            std::cout << "Reading Node with ID: " << read_id << std::endl;
            status = pMyClient->read_specific_nodeid(read_id);
            //status = pMyClient->read();
            UaVariant oldValue(pMyClient->getReadValue());

            UaNodeId objectnodeid = UaNodeId(kks_stream.str().c_str(), 2);
            kks_stream << ".writeSignal";
            UaNodeId methodnodeid = UaNodeId(kks_stream.str().c_str(), 2);



            for (int i = 0; i < anzahl_methodcalls; i++)
            {
                status = pMyClient->callMethodInternal(objectnodeid, methodnodeid);

                auto end = std::chrono::high_resolution_clock::now();

                auto now = std::chrono::system_clock::now();
                std::time_t now_time = std::chrono::system_clock::to_time_t(now);
                auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

                // Zeit formatieren und ausgeben
                std::tm* now_tm = std::localtime(&now_time);
                std::ostringstream oss;
                oss << std::put_time(now_tm, "%Y-%m-%d %H:%M:%S");
                oss << '.' << std::setfill('0') << std::setw(3) << milliseconds.count();

                // Ausgabe
                std::cerr << oss.str() << " -> Signalaenderung gesendet (UATestClient_XU)\n";

                auto start = std::chrono::high_resolution_clock::now();
                if (!status.isGood())
                {
                    fprintf(stderr,"\nFunction \"callMethods()\" was not successfull!\nExit with status code %s.\n", status.toString().toUtf8());
                    break;
                }

                while (1)
                {
                    //status = pMyClient->read();
                    status = pMyClient->read_specific_nodeid(read_id);
                    if (!status.isGood())
                    {
                        fprintf(stderr,"\nFunction \"read()\" was not successfull!\nExit with status code %s.\n", status.toString().toUtf8());
                        break;
                    }

                    if(oldValue != pMyClient->getReadValue())
                    {
                        auto end = std::chrono::high_resolution_clock::now();

                        auto now = std::chrono::system_clock::now();
                        std::time_t now_time = std::chrono::system_clock::to_time_t(now);
                        auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

                        // Zeit formatieren und ausgeben
                        std::tm* now_tm = std::localtime(&now_time);
                        std::ostringstream oss;
                        oss << std::put_time(now_tm, "%Y-%m-%d %H:%M:%S");
                        oss << '.' << std::setfill('0') << std::setw(3) << milliseconds.count();

                        // Ausgabe
                        std::cerr << oss.str() << " -> Signalaenderung detektiert (UATestClient_XU)\n";
                        
                        /*if ((i%50)==0)
                        {
                            fprintf(stderr,"\n%d/%d Operations completed successfully!\n", i, ANZ_METHODCALLS);
                        }*/
                        //printProgressBar(i, anzahl_methodcalls);
                        auto int_s = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
                        times[i] = int_s.count();
                        outfile << "[" << (i+1) << "]: " << int_s.count() << '\n';
                        //oldValue = val2;
                        oldValue = pMyClient->getReadValue();

                        // Random delay
                        // The random delay is built in because the XU works internally with a cycle of about 1s. 
                        // Therefore, if we transmit without a delay, we would virtually emulate this cycle and only receive times of around 1s. 
                        // With the delay, we send with a delay of 0.5s to 2s and therefore receive all possible times.
                        
                        int delay = rand() % 1500;
                        delay += 500;
                        std::this_thread::sleep_for(std::chrono::milliseconds(delay));
                        break;
                    }
                }
            }

            {
                long long int summe = 0;
                for (int i = 0; i < anzahl_methodcalls; i++)
                {
                    summe += times[i];
                }


                outfile << "#\n# Expired test time: [us]" << summe;
                outfile << "\n# Expired test time: [ms]" << summe/1000.0;
                outfile << "\n# Expired test time: [s]" << summe/1000000.0;
                outfile << "\n# Expired test time: [min]" << summe/(1000000*60.0) << "\n#";

                outfile << "\n# Average time: [us] " << ((float)summe / anzahl_methodcalls);
                outfile << "\n# Average time: [ms] " << ((float)summe / anzahl_methodcalls)/1000.0;
            }


            outfile.close();

            /*int retval = system("testcase_roundloop\\testcase_roundloop.py");
            if (retval != 0)
                fprintf(stderr,"\nError opening testcase_roundloop.py\n");*/


            // Disconnect from OPC UA Server
            status = pMyClient->disconnect();
        }
        else if (mode == '3')
        {
            long long int* times = new long long int[anzahl_methodcalls] {};
            std::ofstream outfile("testcase_onlyread.txt");
            std::time_t timenow = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
            float val = 0.0f;
            float step = 0.02f;
            bool upwards = true;
            std::ifstream ifs;
            std::string line;
            std::vector<std::string> vector_string;

            ifs.open("C:/Users/lucaeichenmueller/source/repos/XUtoSTDOUT_simulation/XUtoSTDOUT_simulation/simulation_xu.txt");
            if (ifs.is_open())
            {
                while (std::getline(ifs, line))
                {
                    vector_string.push_back(line);
                }
            }
            else
            {
                std::cerr << "Can't open file!" << std::endl;
                return 1;
            }

            ifs.close();

            for (std::string x : vector_string)
                std::cout << x << '\n';

            //std::cout << "1615650909:84000:f:0:0:0:0:3AHP 712MP               XQ01     \n";
            std::this_thread::sleep_for(std::chrono::milliseconds(10000));

            do{
                status = pMyClient->read();
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            } while (!status.isGood());

            UaVariant oldValue(pMyClient->getReadValue());

            for (int i = 0; i < anzahl_methodcalls; i++)
            {
                // Signal senden
                // 1615650909:84000:f:0:2:12:1491:3AHP 712MP               XQ01     
                //fprintf(stdout, "1615650909:84000:f:%f:0:0:0:3AHP 712MP               XQ01     \n", value);
                //fprintf(stderr, "Sent Signal!");
                
                if (upwards) {
                    val += step;
                    if (val >= 4.0f) {
                        upwards = false;  // Richtung wechseln
                    }
                }
                else {
                    val -= step;
                    if (val <= 1.0f) {
                        upwards = true;  // Richtung wechseln
                    }
                }

                auto start = std::chrono::high_resolution_clock::now();

                while (1)
                {
                    fprintf(stdout, "1615650909:84000:f:%f:0:0:0:3AHP 712MP               XQ01     \n", val);
                    status = pMyClient->read();

                    if (!status.isGood())
                    {
                        fprintf(stderr,"\nFunction \"read()\" was not successfull!\nExit with status code %s.\n", status.toString().toUtf8());
                        break;
                    }
                    

                    if (oldValue != pMyClient->getReadValue())
                    {
                        auto end = std::chrono::high_resolution_clock::now();
                        
                        printProgressBar(i, anzahl_methodcalls);

                        auto int_s = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
                        times[i] = int_s.count();
                        //outfile << "[" << (i + 1) << "]: " << int_s.count() << ": " << atof(pMyClient->getReadValue().toString().toUtf8()) << '\n';
                        outfile << "[" << (i + 1) << "]: " << int_s.count() << '\n';
                        oldValue = pMyClient->getReadValue();
                        break;
                    }
                }
            }

            {
                long long int summe = 0;
                for (int i = 0; i < anzahl_methodcalls; i++)
                {
                    summe += times[i];
                }


                outfile << "#\n# Expired test time: [us]" << summe;
                outfile << "\n# Expired test time: [ms]" << summe / 1000.0;
                outfile << "\n# Expired test time: [s]" << summe / 1000000.0;
                outfile << "\n# Expired test time: [min]" << summe / (1000000 * 60.0) << "\n#";

                outfile << "\n# Average time: [us] " << ((float)summe / anzahl_methodcalls);
                outfile << "\n# Average time: [ms] " << ((float)summe / anzahl_methodcalls) / 1000.0 << '\n';
            }


            outfile.close();


            // Disconnect from OPC UA Server
            status = pMyClient->disconnect();
        }
        else
        {
            std::cerr << "Invalid mode! Disconnecting...\n";
            status = pMyClient->disconnect();
        }
    }


    delete pMyClient;
    pMyClient = NULL;

    // Cleanup the UA Stack platform layer
    UaPlatformLayer::cleanup();
    

    return 0;
}