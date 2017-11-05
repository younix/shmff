# shmff

shmff - *shared memory farbfeld* - is an image processing tool.
It is a prove of concept for fast image manipulation with the unix philosophy in
mind.
shmff uses [farbfeld](https://tools.suckless.org/farbfeld/) as a general image
representation format.

# Introduction

The original farbfeld tools utilize the shell piping mechanism to orchestrate an
image manipulation tool chain.
Thus, image processing is as easy as text processing.
Instead of one bloated tool that tries to solve every problem, it is tools suite
of single program for specific tasks.

```
jpg2ff input.jpg | resize 1280 1024 | grey | ff2png output.png
```

Piping image data from one process to another is the main disadvantage of
farbfeld.
Every pixel has to be copied through the piping buffer of the kernel.
Thus, a pixel has to be read and written twice before it is transferred from
one process to another.

shmff solves this problem by keeping the image data in memory and shares the
access of this memory area with other processes.
First, a program allocated a shared memory area and loads an image into it.
Than, it communicates the identifier of this memory are to the next process
via stdin/stdout.

```
ff2shm input.ff | resize 1280 1024 | grey | shm2ff output.ff
```

Thus, the interprocess communication effort is reduced to a minimum and the
user interace is as simple as of the original farbfeld tool suite.

# Tools

The following programs are the core tools of this project.
They load/store a farbfeld image into/from memory/file system.

 * **ff2shm** loads a farbfeld file into a shared memory area.
 * **shm2ff** saves a sharedff memory area into a farbfeld file.

## ff2shm

**ff2shm** is a source of a shmff command chain.
It creates a new shared memory area and loads a complete farbfeld file into a
this memory.
Then, ff2shm converts all integer values into host byte order.
Finally, it writes a data shmff structure to stdout and exits.

## shmff data structure

The shmff data structure is host-wide identifier for a loaded shmff memory
area.
The magic string "sharedff" indicates this data structure.
Its size is architecture dependent because key\_t and size\_t are.

```
+-----------------+-------+------+
| s h a r e d f f | key_t | size |
+-----------------+-------+------+

Example in C:

struct {
	char magic[8];
	key_t key;
	size_t size;
};
```

## shm2ff

**shm2ff** a shared farbfeld sink into file system.
It reads the shmff data structure from stdin mand maps the corresponding
memory page.
Then, it converts all integer values into network byte order and writes the
image data into a file.
Finally, it detaches and destroys the shared memory area.

# Image Manipulation Tools

The following programs are converters which manipulate the shared ff images.
The programs read a shared ff descriptor from stdin.
The programs have to write a new shared ff descriptor to stdout.

 * **gray** convert color to gray
 * **invert** invert colors
 * **gaus** gausian blur
 * **crop** crop image
 * **dummy** does nothing.  It is just an API example.

# Calling Convention

A converter program reads a struct shmff from stdin.
This structure contains all infomation to map shared memory area with the image
into the virtual memomy of the converter programm.
The program is allowed to create a new shared memory area with the destination
image.
In this case the program is responsible for the old memory area.
The progam have to free this area.
After the conversion is done, the program have to write to corresponding shmff
structure to stdout.

**In cases of an error, the program have to delete all shared memory pages.**

# Performance

**CAUTION: outdated**

 * 242 cycles per 10sec on iMac
 *  97 cycles per 10sec on X1 carbon
