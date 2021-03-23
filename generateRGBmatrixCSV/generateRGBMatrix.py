#!/usr/local/bin/python3
from PIL import Image
import argparse
import os
import pathlib

if __name__ == "__main__":
    parser = argparse.ArgumentParser(prog='generateRGBMatrix.py', description='Generate CSV files with RGB matrix info', formatter_class=lambda *args, **kwargs: argparse.HelpFormatter(*args, max_help_position=80, **kwargs))
    parser.add_argument('folderIn', type=str, help='Path to folder', metavar='Input')
    parser.add_argument('folderOut', type=str, help='Path to output folder (it is not necessary that it previously exists)', metavar='Output')

    args = parser.parse_args()

    # Create folder for output files
    pathOut = str(pathlib.Path(__file__).parent.absolute()) + '/' + args.folderOut
    os.makedirs(pathOut, exist_ok=True)

    # Generate matrix for every file in folderIn
    pathIn = str(pathlib.Path(__file__).parent.absolute()) + '/' + args.folderIn
    print(pathIn)
    files = os.listdir(args.folderIn)
    for file in files:
        print('Starting dealind with ' + file)
        img = Image.open(pathIn + str(file))
        with open(pathOut + '/' + file[:-4] + '.h', 'wt') as csvFile:
            csvFile.write('const char* ' + file[:-4] + ' = ')
            csvFile.write('"WIDTHINDEX,HEIGHTINDEX,RED,GREEN,BLUE\\n"\n')
            for widthIndex in range(8):
                for heightIndex in range(8):
                    colors = img.getpixel((widthIndex, heightIndex))
                    csvFile.write('"' + str(widthIndex) + ',' + str(heightIndex) + ',' + str(colors[0]) + ',' + str(colors[1]) + ',' + str(colors[2]) + '\\n"\n') 
            csvFile.write(';')
        print('Finished with ' + file)

    print('Writing everyRGB.h file')
    with open(pathOut + '/everyRGB.h', 'wt') as f:
        for file in files:
            f.write('#include ' + file[:-4] + '.h\n')

    print('everyRGB.h file written')

    # Other choice is to write everything in just one file

    print('Writing allRGB.h file')
    with open(pathOut + '/allRGB.h', 'wt') as f:
        for file in files:
            img = Image.open(pathIn + str(file))
            f.write('const char* ' + file[:-4] + ' = ')
            f.write('"WIDTHINDEX,HEIGHTINDEX,RED,GREEN,BLUE\\n"\n')
            for widthIndex in range(8):
                for heightIndex in range(8):
                    colors = img.getpixel((widthIndex, heightIndex))
                    f.write('"' + str(widthIndex) + ',' + str(heightIndex) + ',' + str(colors[0]) + ',' + str(colors[1]) + ',' + str(colors[2]) + '\\n"\n') 

            f.write(';\n')
        print('Finished with ' + file)

    print('allRGB.h file written')