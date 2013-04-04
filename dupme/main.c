#include <unistd.h>
#include <stdlib.h>

int str_to_int(char *s)
{
    int result = 0;
    while (*s != 0)
    {
        result = 10 * result + (*s - '0');
        s++;
    }
    return result;
}

int main(int argc, char** argv) 
{
	int k, length = 0, eof_reached = 0, ignore = 0;
	int readed, i, j, start_write, written, count;
	
	k = str_to_int(argv[1]) + 1;
	if (k < 2 || argc != 2) 
		return 1;
	char* input_data = (char *) malloc(k + 1);
	while (1)
	{
		readed = read(0, input_data + length, k - length);
		if (readed == 0)
		{
			if (length > 0 && ignore == 0)
			{
				input_data[length] = '\n';
				readed = length + 1;
				length = 0;
			}
			eof_reached = 1;
		}
		i = length, start_write = 0, written = 0;

		for (; i < length + readed; i++)
			if (input_data[i] == '\n')
			{
				if (ignore == 1)
				{
					start_write = i + 1;
					ignore = 0;
				}
				count = i + 1 - start_write, j = 0;
		        for (; j < 2; j++)
		        {
		        	written = 0;
			        while (written < count)
			            written += write(1, input_data + written, count - written);
		    	}
				start_write = i + 1;
			}

		if (eof_reached == 1)
			return 0;

		memmove(input_data, input_data + start_write, length + readed - start_write);
		length += readed - start_write;

		if (length == k)
		{
			ignore = 1;
			length = 0;
		}
	}
	free(input_data);
	return 0;
}