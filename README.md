# shmff

shmff - shared memory farbfeld - is a ff image processing tool.  It is prove of
concept for fast image manipulation with Unix philosophy.  shmff uses farbfeld
as an image representation format.

# programs

The following programms are the core tools of this projekt.
They load and store the ff images into/from memory.

 * **ff2shm** loads a ff image into shared ff.
 * **shm2ff** saves a sharedff into a ff file.

The following programms are converters which manipulate the shared ff images.
The programs read a shared ff descriptor from stdin.
The programs have to write a new shared ff descriptor to stdout.

 * **gray** convert color to gray
 * **invert** invert colors
 * **gaus** gausian blur
 * **crop** crop image
 * **dummy** does nothing.  It is just an API example.

# calling convention

A converter program reads a struct shmff from stdin.
This structure contains all infomation to map shared memory area with the image
into the virtual memomy of the converter programm.
The program is allowed to create a new shared memory area with the destination
image.
In this case the program is responsible for the old memory area.
The progam have to free this area.
After the conversion is done, the program have to write to corresponding shmff
structure to stdout.

In cases of an error, the program have to delete all shared memory pages.

# performance

 * 242 cycles per 10sec on iMac
 *  97 cycles per 10sec on X1 carbon
