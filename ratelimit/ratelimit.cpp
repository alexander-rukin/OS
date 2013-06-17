#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <iostream>
#include <fcntl.h>
#include <string.h>
#include <chrono>
#include <math.h>
#include <vector>

#define MAX_SIZE 4096
#define MILL 1000000

using namespace std;

struct RateLimit
{
	int n;
	char* buffer;
	vector<pair<char*,int>> v;

	RateLimit(size_t n):n(n)
	{

		buffer = (char*)malloc(MAX_SIZE);
		if (buffer == NULL)
		{
			exit(0);
		}
	}


	void run()
	{
		int filed = 0;
		while(true)
		{
			int i = 0;
			auto start = chrono::high_resolution_clock::now();
			int incomming = 0;

			if (filed == 0)
				incomming = read (0, buffer + filed, MAX_SIZE - filed);
			if (incomming < 0) exit(0);
			if (incomming == 0 && filed == 0) break;

			filed += incomming;
			
			i = 0; 

			while (i < filed)
			{
				if (buffer[i] == '\n')
				{
					char *string = (char*)malloc(i + 1);
					memcpy(string, buffer, i + 1);
					v.push_back(make_pair(string, i+1));
					filed -= (i + 1);
					memmove(buffer, buffer + i + 1, filed);
					i = -1;
				}
				if (v.size() == n)
					break;
				i++;
			}

			if (v.size() == n )
			{
				auto finish = chrono::high_resolution_clock::now();
				double timer = chrono::duration<double>(finish - start).count();
				int sleep_time = MILL - ((int)timer*MILL)%MILL;
				usleep(sleep_time);

				for (int j = 0; j < v.size(); j++)
					write(1, v[j].first, v[j].second);
				v.clear();
			} else if (v.size() < n && v.size() > 0) {
                auto finish = chrono::high_resolution_clock::now();
				double timer = chrono::duration<double>(finish - start).count();
				int sleep_time = MILL - ((int)timer*MILL)%MILL;
				usleep(sleep_time);
                
				for (int j = 0; j < v.size(); j++)
					write(1, v[j].first, v[j].second);
				v.clear();
			}
		}
	}


	~RateLimit()
	{
		
		if(buffer)
			free(buffer);
	}
};


int main(int argc , char ** argv) 
{
	if (argc < 1) {
        return 2;
    }

	RateLimit rl(atoi(argv[1]));
	rl.run();

	return 0;
}