# edge_detector

This is an image preprocessing program that performs edge detection on ppm images. This is a multithreaded program which can be used to perform edge_detection on multiple files in a directory. It also utilizes multiple threads to process each individual images in order to reduce processing time. The number of threads in use can be adjusted. 

Along with the program itself, I ran some experiments with Bash scripts to test the effect of increasing threads on multiple systems. Here are some interesting results from those experiments, where I show the intuitive result that the benefits of multithreading are best enjoyed when more CPU cores are in use.


![8core](https://github.com/user-attachments/assets/2d9d2dc2-7a21-4192-9028-f8fd9341d9c9)
![16core](https://github.com/user-attachments/assets/ac5224d0-6559-492a-ba34-ed077d09cea4)
![32core](https://github.com/user-attachments/assets/52f12f24-32cd-45ee-8c72-1ad734328540)
