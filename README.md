PDP Coursework
==============

To compile the code just type:

	$ make

The code can be run with the following example:

	$ mpirun -np 120 ./frogs_VS_yellow_tongues -f 34 -i 4 -y 100 -l 120
	
In order to run it on the backend of Morar just type:

	$ qsub submit.sge

To see full details of the arguments type:

	$ mpirun -np 120 ./frogs_VS_yellow_tongues -h