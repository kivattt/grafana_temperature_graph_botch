g++ -std=c++2a -Wall -c main.cpp webcam-v4l2/webcam.cpp && g++ -std=c++2a -Wall main.o webcam.o -o main -lv4l2 -lsfml-graphics -lsfml-window -lsfml-system -lsfml-audio -lsfml-network
