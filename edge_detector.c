/* edge_detector is an multithreaded image processing program that detects discontinuties in a P6 PPM image.
 * This program uses Laplacian edge detection to filter pixels, and creates a filtered output P6 PPM image 
 * after processing is completed. Multiple files can be simultaneously processed at a time by listing 
 * each filename to be processed (eg. ./edge_detector file1.ppm file2.ppm ... fileN.ppm).
 * Output image files will be created in the directory where edge_detector was invoked.
 * Author: Cameron Henderson 
 * Date: March 2024
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>
#include <pthread.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>

#define LAPLACIAN_THREADS 4    

/* Laplacian filter is 3 by 3 */
#define FILTER_WIDTH 3       
#define FILTER_HEIGHT 3      

#define RGB_COMPONENT_COLOR 255

typedef struct {
      unsigned char r, g, b;
} PPMPixel;

struct parameter {
    PPMPixel *image;         //original image pixel data
    PPMPixel *result;        //filtered image pixel data
    unsigned long int w;     //width of image
    unsigned long int h;     //height of image
    unsigned long int start; //starting point of work
    unsigned long int size;  //equal share of work (almost equal if odd)
};


struct file_name_args {
    char *input_file_name;      //e.g., file1.ppm 
    char output_file_name[64];  //will take the form laplaciani.ppm, e.g., laplacian1.ppm
};


/*The total_elapsed_time is the total time taken by all threads 
to compute the edge detection of all input images .
*/
double total_elapsed_time = 0; 
pthread_mutex_t mtx_etime; // mutex to lock total_elapsed_time


/*This is the thread function. It will compute the new values for the region of image specified in params (start to start+size) 
	using convolution. For each pixel in the input image, the filter is conceptually placed on top ofthe image with its origin
    lying on that pixel. The  values  of  each  input  image  pixel  under  the  mask  are  multiplied  by the corresponding 
	filter values. Truncate values smaller than zero to zero and larger than 255 to 255. The results are summed together to 
    yield a single output value that is placed in the output image at the location of the pixel being processed on the input.
 
 */
void *compute_laplacian_threadfn(void *params)
{
    struct parameter* p = (struct parameter*) params;
	PPMPixel *image = p->image;
	PPMPixel *result = p->result;
	unsigned long w = p->w;
	unsigned long h = p->h;
	unsigned long start = p->start;
	unsigned long size = p->size;

    int laplacian[FILTER_WIDTH][FILTER_HEIGHT] =
    {
        {-1, -1, -1},
        {-1,  8, -1},
        {-1, -1, -1}
    };

    int red, green, blue;   
	unsigned long x_coordinate;
	unsigned long y_coordinate;
	for (unsigned long img_y = start; img_y < start + size; img_y++) {
		for (unsigned long img_x  = 0; img_x < w; img_x++) {
			red = 0;
			green = 0;
			blue = 0;
			for (int filter_x = 0; filter_x < FILTER_WIDTH; filter_x++) {
				for (int filter_y = 0; filter_y < FILTER_HEIGHT; filter_y++) {
					x_coordinate = (img_x - FILTER_WIDTH / 2 + filter_x + w) % w;
					y_coordinate = (img_y - FILTER_HEIGHT / 2 + filter_y + h) % h;
					red += image[y_coordinate * w + x_coordinate].r * laplacian[filter_y][filter_x];
					green += image[y_coordinate * w + x_coordinate].g * laplacian[filter_y][filter_x];
					blue += image[y_coordinate * w + x_coordinate].b * laplacian[filter_y][filter_x];
				}
			}
			// restrict colors to values between 0 and RGB_COMPONENT_COLOR
			red = red < 0 ? 0 : red;
			red = red > RGB_COMPONENT_COLOR ? RGB_COMPONENT_COLOR : red;		
			green = green < 0 ? 0: green;
			green = green > RGB_COMPONENT_COLOR ? RGB_COMPONENT_COLOR : green;		
			blue = blue < 0 ? 0 : blue;
			blue = blue > RGB_COMPONENT_COLOR ? RGB_COMPONENT_COLOR : blue;			

			result[img_y * w + img_x].r = red; 
			result[img_y * w + img_x].g = green; 
			result[img_y * w + img_x].b = blue;
		}
	}	
    return NULL; // nothing to return
}

/* Apply the Laplacian filter to an image using threads.
 Each thread shall do an equal share of the work, i.e. work=height/number of threads. If the size is not even, 
 the last thread shall take the rest of the work.Compute the elapsed time and store it in *elapsedTime (Read about gettimeofday).
 Return: result (filtered image). The caller is responsible for freeing result.
 */
PPMPixel *apply_filters(PPMPixel *image, unsigned long w, unsigned long h, double *elapsedTime) {
	// start elapsed time
	struct timeval start_time;
	if (gettimeofday(&start_time, NULL)) perror("gettimeofday");

	int num_threads = (h / LAPLACIAN_THREADS) < 1 ? h : LAPLACIAN_THREADS; // cap number of threads to the height of the image - prevents threads from doing zero work
	pthread_t threads[num_threads];
	PPMPixel *result = malloc(w * h * sizeof(PPMPixel));
	if (!result) {
		perror("malloc");
		return NULL;
	}

	struct parameter* params = (struct parameter*) malloc(num_threads * sizeof(struct parameter));
	if (!params) {
		perror("malloc");
		return NULL;
	}
	
	// Split image processing between evenly between threads. 
	// Last thread takes care of what's left, in case of an odd number of lines to process.
	int i;
    for (i = 0; i < num_threads - 1; i++) {
		params[i].image = image;
		params[i].result = result;
		params[i].w = w;
		params[i].h = h;
		params[i].size = h/num_threads;
		params[i].start = i * params[i].size;
		pthread_create(&threads[i], NULL, &compute_laplacian_threadfn, (void*)&params[i]);
	}   
	params[i].image = image;
	params[i].result = result;
	params[i].w = w;
	params[i].h = h;
	params[i].start = i * (h/num_threads);
	params[i].size = h - params[i].start;
	pthread_create(&threads[i], NULL, &compute_laplacian_threadfn, (void*)&params[i]);

	for (int i = 0; i < num_threads; i++) {
		int err = pthread_join(threads[i], NULL);
		if (err) { 
			fprintf(stderr, "pthread_join failure: %s", strerror(err));
			return NULL;
		}
	}

	// end elapsed time
	struct timeval end_time;
	if (gettimeofday(&end_time, NULL)) perror("gettimeofday");
	*elapsedTime = (double)end_time.tv_sec + ((double)end_time.tv_usec / 1000000) - (double)start_time.tv_sec - ((double)start_time.tv_usec / 1000000);
	free(params);
    return result;
}

/*Create a new P6 file to save the filtered image in. Write the header block
 e.g. P6
      Width Height
      Max color value
 then write the image data.
 The name of the new file shall be "filename" (the second argument).
 */
void write_image(PPMPixel *image, char *filename, unsigned long int width, unsigned long int height)
{
	FILE* outfile;
	outfile = fopen(filename, "w");
	if (outfile == NULL) {
		fprintf(stderr, "\"%s\": write file error: %s\n", filename, strerror(errno));
		return;
	}
	
	fprintf(outfile, "P6\n");
	fprintf(outfile, "# Cameron Henderson Western Washington University CSCI347\n");
	fprintf(outfile, "%lu ", width);
	fprintf(outfile, "%lu\n", height);
	fprintf(outfile, "%d\n", RGB_COMPONENT_COLOR);

	int pixels_written = 0;
	for (int i = 0; i < width * height; i++) {
		pixels_written += fwrite(&image[i], sizeof(PPMPixel), 1, outfile);
	}
	if (pixels_written < (width * height)) {
		fprintf(stderr, "error writing to destination file \"%s\"\n", filename);
	}
	return;
}

/* Copy data from the stream into buffer 'buf' until whitespace is reached. 
 * A terminating null character is appended to the end of the characters in buf.
 * Any lines starting with # symbol will be skipped.
 * Return: number of characters written into buf, excluding the terminating null.
 */
static int getnextchunk(FILE* file, char* buf, int bufsiz) {
	char c = fgetc(file);
	while ( c == '#' && c != EOF ) {
		while ( c != '\n' && c != EOF ) { 
			c = fgetc(file);
		}
		if ( c != EOF) {
			c = fgetc(file);
		}
	}
	if ( c == EOF) {
		ungetc(c, file);
		return 0; 
	}
	int i = 0; 
	while ( !isspace(c) && c != EOF && i < bufsiz - 1) {
		buf[i] = c;
		c = fgetc(file);
		i++;
	}
	buf[i] = '\0';
	while ( isspace(c)) {
		c = fgetc(file);
	}	
	ungetc(c, file);
	return i;
}


/* Open the filename image for reading, and parse it.
    Example of a ppm header:    //http://netpbm.sourceforge.net/doc/ppm.html
    P6                  -- image format
    # comment           -- comment lines begin with
    ## another comment  -- any number of comment lines
    200 300             -- image width & height 
    255                 -- max color value
 
 Check if the image format is P6. If not, print invalid format error message.
 If there are comments in the file, skip them. You may assume that comments exist only in the header block.
 Read the image size information and store them in width and height.
 Check the rgb component, if not 255, display error message.
 Return: pointer to PPMPixel that has the pixel data of the input image (filename).The pixel data is stored in scanline
 order from left to right (up to bottom) in 3-byte chunks (r g b values for each pixel) encoded as binary numbers.
 On failure, return NULL (eg the filename does not exist, the header is not a valid P6 image header, 
 or there is an error while reading the file).
 The caller is responsible for freeing the return img pointer.
 */
PPMPixel *read_image(const char *filename, unsigned long int *width, unsigned long int *height)
{
    PPMPixel *img;
	FILE* infile;	
	// open file for read-only
	infile = fopen(filename, "r");
	if (infile == NULL) {
		fprintf(stderr, "\"%s\": image header read error: %s\n", filename, strerror(errno));
		return NULL;
	}

	char magic_num[32];
	char width_str[32];
	char height_str[32];
	char maxcolor_str[32];
	int rgb;
	// get magic number
	getnextchunk(infile, magic_num, 16);
	if (strcmp(magic_num, "P6") != 0) {
		fprintf(stderr, "\"%s\": image header read error: magic number does not match P6\n", filename);
		return NULL;
	}	
	// get width
	getnextchunk(infile, width_str, sizeof(width_str));
	char* endptr;
	errno = 0;
	*width = strtol(width_str, &endptr, 10);
	if (errno != 0) {
		perror("strtol");
		return NULL;
	}
	if (endptr == width_str) {
		fprintf(stderr, "\"%s\": image header read error: no digits found for width\n", filename);
		return NULL;
	}
	// get height
	getnextchunk(infile, height_str, sizeof(height_str));
	errno = 0;
	*height = strtol(height_str, &endptr, 10);
	if (errno != 0) {
		perror("strtol");
		return NULL;
	}
	if (endptr == height_str) {
		fprintf(stderr, "\"%s\": image header read error: no digits found for height\n", filename);
		return NULL;
	}
	// get max color value
	getnextchunk(infile, maxcolor_str, sizeof(maxcolor_str));
	errno = 0;
	rgb = strtol(maxcolor_str, &endptr, 10);
	if (errno != 0) {
		perror("strtol");
		return NULL;
	}
	if (endptr == height_str) {
		fprintf(stderr, "\"%s\": image header read error: no digits found for max rgb color value\n", filename);
		return NULL;
	}	
	if (rgb != RGB_COMPONENT_COLOR) {
		fprintf(stderr, "\"%s\": image header read error: maximum rgb color value must be %d\n", filename, RGB_COMPONENT_COLOR);
		return NULL;
	}
	
	int pixelarea = (*width) * (*height);
	img = calloc( pixelarea, sizeof(PPMPixel));
	if (!img) {
		perror("malloc");
		return NULL;
	}
	int total_pixels_read = 0;
	for (int i = 0; i < pixelarea; i++) {
		int pixelread = fread(&img[i], sizeof(PPMPixel), 1, infile);
		total_pixels_read += pixelread;
	}
	if (total_pixels_read < pixelarea && !feof(infile)) {
		fprintf(stderr, "\"%s\": input image read error: expected pixels: %d, pixels read: %d\n", filename, pixelarea, total_pixels_read);
		return NULL;
	}
    return img;
}

/* The thread function that manages an image file. 
 Read an image file that is passed as an argument at runtime. 
 Apply the Laplacian filter. 
 Update the value of total_elapsed_time.
 Save the result image in a file called laplaciani.ppm, where i is the image file order in the passed arguments.
 Example: the result image of the file passed third during the input shall be called "laplacian3.ppm".
*/
void *manage_image_file(void *args) {
	struct file_name_args* arguments = (struct file_name_args*) args;
	unsigned long w;
	unsigned long h;
	double  elapsedTime = 0;
	PPMPixel *input_img;
	PPMPixel *output_img;
	input_img = read_image(arguments->input_file_name, &w, &h); // must free after use
	if (input_img) {
		output_img = apply_filters(input_img, w, h, &elapsedTime); // must free after use
		write_image(output_img, arguments->output_file_name, w, h); 	
		printf("Input image: %s, Output image: %s, Elapsed time: %f\n", arguments->input_file_name, arguments->output_file_name, elapsedTime);
	} else {
		fprintf(stderr, "\"%s\": input image read error, no output image created\n", arguments->input_file_name);
	}
	pthread_mutex_lock(&mtx_etime);
	total_elapsed_time += elapsedTime;
	pthread_mutex_unlock(&mtx_etime);
	free(input_img);
	free(output_img);
	return NULL; //nothing to return?
}

/*The driver of the program. Check for the correct number of arguments. If wrong print the message: "Usage ./a.out filename[s]"
  It shall accept n filenames as arguments, separated by whitespace, e.g., ./a.out file1.ppm file2.ppm    file3.ppm
  It will create a thread for each input file to manage.  
  It will print the total elapsed time in .4 precision seconds(e.g., 0.1234 s). 
 */
int main(int argc, char *argv[])
{
	printf("LAPLACIAN THREADS: %d\n", LAPLACIAN_THREADS);
	if (argc < 2) {
		fprintf(stderr, "Usage: ./edge_detector filenames[s]\n");
		return EXIT_FAILURE;
	}
	pthread_mutex_init(&mtx_etime, NULL);
	int num_threads = argc - 1;
	pthread_t threads[num_threads];
	struct file_name_args* args = malloc(num_threads * sizeof(struct file_name_args));
	if (!args) {
		perror("malloc");
		return EXIT_FAILURE;
	}
	for (int i = 0; i < num_threads; i++) {
		args[i].input_file_name = argv[i+1];
		snprintf(args[i].output_file_name, sizeof args[i].output_file_name, "laplacian%d.ppm", i+1); 
		pthread_create(&threads[i], NULL, &manage_image_file, (void*)&args[i]);
	}
	for (int i = 0; i < num_threads; i++) {
		int err = pthread_join(threads[i], NULL);
		if (err) 
			fprintf(stderr, "pthread_join error: %s", strerror(err));
	}
	printf("Total elapsed time: %.4f\n", total_elapsed_time);
	free(args);
	pthread_mutex_destroy(&mtx_etime);
    return 0;
}

