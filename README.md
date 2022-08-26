## Overview
A basic zip application compresses a file with run-length encoding as the basic
technique.

This application of parallel zip (`pzip`) will externally look the same; the general usage
from the command line will be as follows:

```
prompt> ./pzip file > file.z
```

There may also be many input files (not just one, as above). However,
internally, the program will use POSIX threads to parallelize the compression
process.  




