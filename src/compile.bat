@echo off
g++ -shared -O3 -Os -s -o .\libphics.dll -mavx2 -mfma .\libphics.cpp
