#include <iostream>
#include <poll.h>
#include <errno.h>
#include <string>
#include <cstdlib>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <cstdio>
#include <netdb.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <chrono>
#include <vector>

#define MILL 1000000
#define FIVE_SEC 5000000
#define MAX_SIZE 4096
#define AWESOME_TEMP 30
#define DELTA 3

using namespace std;

pid_t pid;

void handler(int) {

    kill(pid, SIGINT);
}


int max(int a, int b, int c)
{

    if (a >= b && a >= c)
        return a;
    else if (b >=a && b >= c)
        return b;
    else
        return c;
}

int min(int a, int b)
{
    if (a < b)
        return a;
    else
        return b;
}

int max(int a, int b)
{
    if (a > b)
        return a;
    else
        return b;
}

int read_int(int fd)
{
    
    int res, cur = 0;
    char buf[16];
    while ((res = read(fd, buf + cur, 16 - cur)) > 0)
        cur += res;
    buf[cur +1] = '\0';
    return atoi(buf);
}


bool write_int(int fd, int w)
{

    int res, cur = 0;
    char buf[16];
    sprintf(buf, "%d", w);
    int ms = strlen(buf);
    while ((res = write(fd, buf + cur, ms - cur)) > 0)
        cur += res;
    write(fd, "\n", 1);
    return res >=0;
}

int cooler_func_1 (int in, vector<int> temp)
{
    in -= AWESOME_TEMP - ((temp[0] + temp[1] + temp[2]) / 3);
    return min(in, 255);
}

int cooler_func_2 (int in, vector<int> temp)
{
    in -= AWESOME_TEMP - (max(temp[0], temp[1], temp[2]));
    return min(in, 255);
}

int main(int argc, char* argv[])
{
    
    pid = fork();
    if (pid)
    {
        signal(SIGINT, handler);
        printf("Started daemon, pid: %d\n", pid);
        int status;
        waitpid(pid, &status, 0);
        printf("\nDaemon stopped\n");
    } else {
        int log_fd = dup(STDOUT_FILENO);
        if (log_fd == -1)
        {
            _exit(43);
        }
        
//        close(STDIN_FILENO);
//        close(STDOUT_FILENO);
//        close(STDERR_FILENO);
        setsid();

        vector<int> fans, temp;
        const char* file_name = "cooler.txt";
        
        vector<char*> file_names;
        vector<char*> temp_names;
        
//        file_names.push_back("/sys/class/hwmon/hwmon0/device/fan1_input");
//        file_names.push_back("/sys/class/hwmon/hwmon0/device/fan2_input");
        
//        temp_names.push_back("/sys/class/hwmon/hwmon0/temp1_input");
//        temp_names.push_back("/sys/class/hwmon/hwmon0/temp2_input");
//        temp_names.push_back("/sys/class/hwmon/hwmon0/temp3_input");
        
        file_names.push_back("fan1.txt");
        file_names.push_back("fan2.txt");

        temp_names.push_back("temp1.txt");
        temp_names.push_back("temp2.txt");
        temp_names.push_back("temp3.txt");
        
        fans.push_back(0);
        fans.push_back(0);
        
        temp.push_back(0);
        temp.push_back(0);
        temp.push_back(0);
        
        char* buffer = (char*)malloc(MAX_SIZE);
        if (buffer == NULL)
        {
            exit(0);
        }
        int temp_temp = 0;
        int delta = 0;
        while (true)
        {
            auto start = chrono::high_resolution_clock::now();
            
            for (size_t i = 0; i < temp_names.size(); i++) {
                int fr = open(temp_names[i], O_RDONLY, S_IRUSR | S_IWUSR);
                temp_temp = temp[i];
                temp[i] = read_int(fr);
                delta = max(temp_temp-temp[i], delta);
                close(fr);
            }
            
            for (size_t i = 0; i < file_names.size(); i++) {
                int fr = open(file_names[i], O_RDONLY, S_IRUSR | S_IWUSR);
                fans[i] = read_int(fr);
                close(fr);
            }
            
            if (delta < DELTA) {
                fans[0] = cooler_func_1(fans[0], temp);
                fans[1] = cooler_func_2(fans[1], temp);
            }
            
            for (size_t i = 0; i < file_names.size(); i++) {
                int fw = open(file_names[i], O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
                write_int(fw, fans[i]);
                close(fw);
            }
            
            auto finish = chrono::high_resolution_clock::now();
            double timer = chrono::duration<double>(finish - start).count();
            int sleep_time = FIVE_SEC - ((int)timer*MILL)%FIVE_SEC;
            usleep(sleep_time);
            printf("\nNext step\n");
        }
        
    }
}
