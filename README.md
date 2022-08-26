Arnav Mehta, anmehta4
Anvit Thekatte, anvit

I stored all the files first in a 2D char* array.
Each index had a file which then had an array of all the characters.
I Iterated through this array with chucnks of data roughly split up by num_of_threads
Finally I let all the computation of the the frequency of the data happen concurrently 
Using a CV and state variable ( myturn ) I ensured the threads run sequencitally while writing to the file.

