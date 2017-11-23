//
// Created by Nuno Humberto on 19/11/2017.
//

#include "RealAudioCodec.h"

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <map>
#include <vector>
#include <string>
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <fstream>
#include <sndfile.hh>


void RealAudioCodec::writeWavToFile(string filename, vector<short>& left, vector<int>& delta, int chan) {
    short *sample = new short[2];
    SndfileHandle soundFileOut = SndfileHandle(filename, SFM_WRITE, SF_FORMAT_WAV | SF_FORMAT_PCM_16, chan, 44100);
    cout << "Writing decoded file to: " << filename << endl;
    vector<short> all;
    if(chan == 2) {
        for(int i = 0; i < left.size(); i++) {
            all.push_back(left.at(i));
            all.push_back((short) (left.at(i) - delta.at(i)));
        }
    } else {
        for(int i = 0; i < left.size(); i++) {
            all.push_back(left.at(i));
        }
    }


    sf_count_t totalsize = all.size() * sizeof(short);
    soundFileOut.writeRaw(all.data(), totalsize);

}


void RealAudioCodec::flush(ofstream& file) {
    char towrite[1];
    while (bitsinbuffer >= 8) {
        towrite[0] = (char) ((buffer >> (bitsinbuffer - 8)) & 0xFF);
        //towrite[0] = (char) 0xBA;
        //if(ctr > 0) cout << "Writing: " << hex << (int) towrite[0] << dec << endl;
        //if(ctr > 0) cout << "Buffer: " << hex << buffer << dec << " (" << bitsinbuffer << ")" << endl;
        file.write(towrite, 1);
        bitsinbuffer -= 8;
        buffer = buffer & buildOnes(bitsinbuffer);

    }
}



void RealAudioCodec::forceFlush(ofstream& file) {
    char towrite[1];
    while (bitsinbuffer >= 8) {
        towrite[0] = (char) ((buffer >> (bitsinbuffer - 8)) & 0xFF);
        file.write(towrite, 1);
        bitsinbuffer -= 8;
        buffer = buffer & buildOnes(bitsinbuffer);
    }
    if (bitsinbuffer > 0) {
        towrite[0] = (char) ((buffer << (8 - bitsinbuffer)) & 0xFF);
        file.write(towrite, 1);
        buffer = 0;
        bitsinbuffer = 0;
    }

}

void RealAudioCodec::writeToFile(int data, int length, char type, ofstream& file) {

    if (type == 'q') {
        int length_to_go = length;
        while(length_to_go >= 8) {
            buffer = (buffer << 8) | 0xFF;
            bitsinbuffer += 8;
            flush(file);
            length_to_go -= 8;
        }
        if(length_to_go > 0) {
            buffer = ((buffer << length_to_go) | ((0x00FF >> (8 - length_to_go)) & 0xFF));
            bitsinbuffer += length_to_go;
            flush(file);
        }
        buffer = buffer << 1;
        bitsinbuffer += 1;
        flush(file);
    }
    else if (type == 'r') {
        buffer = (buffer << length) | (data & buildOnes(length));
        bitsinbuffer += length;
        flush(file);
    }
    else if (type == 'p') {
        buffer = (buffer << length) | (data & 0x03);
        bitsinbuffer += length;
        flush(file);
    }
    else if (type == 'm') {
        buffer = (buffer << length) | (data & 0xFF);
        bitsinbuffer += length;
        flush(file);
    }
    else if (type == 'f') {
        forceFlush(file);
    }
}

void RealAudioCodec::encodeGolombToFile(vector<int>& input, vector<int>& predictors, vector<int>& m_vector, ofstream& outfile, int bs, int factor) {
    vector<int>::iterator it;
    int target;
    int q, r;
    int nbits;// = (int) log2(m);
    int m;
    int counter = 100;
    //long estsize = 0;
    //cout << "R values: ";
    int i = 0, blockcounter = 0, partition_index = 0;
    for(it = input.begin(); it != input.end(); it++, i++) {

        if(*it < 0) target = -(1 + *it * 2);
        else target = 2 * *it;
        //if(counter-- > 0) cout << *it << " ";
        if ((i % bs) == 0) {
            if((blockcounter % factor) == 0) {

                m = m_vector.at(partition_index++);

                nbits = (int) log2(m);


                writeToFile(nbits, 8, 'm', outfile);
                //estsize += 8;
            }

            blockcounter++;

            writeToFile(predictors.at(i/bs), 2, 'p', outfile);
            //estsize += 2;
        }



        q = target/m;

        writeToFile(q, q, 'q', outfile);
        //estsize += (q+1);

        r = target - q * m;

        //if(blockcounter == 52) cout << predictors.at(i/bs) << " ";

        writeToFile(r, nbits, 'r', outfile);

        //estsize += nbits;
    }
    //cout << endl;

    writeToFile(0, 0, 'f', outfile);
    //cout << "Wrote a total of " << partition_index << " partitions.\n";
    //cout << "Estimated size: " << estsize/8 << endl;
}

vector<int> RealAudioCodec::decodeGolombFromFile(ifstream& input, long total_samples, int bs, int factor) {
    vector<int> output;
    int blockcounter = 0;
    int m, nbits, predictor, q, r, original;
    int value;
    int lastvals[4] = {0};
    int counter = 100;
    //cout << "R values: ";
    for(long readsamples = 0; readsamples < total_samples; readsamples++) {
        if ((readsamples % bs) == 0) {
            if ((blockcounter % factor) == 0) {
                nbits = (int) readNextByte(input);

                m = pow(2, nbits);
            }
            predictor = readNextPredictor(input);
            blockcounter++;
        }
        q = readNextQ(input);

        r = readNextR(input, nbits);

        value = m*q + r;



        if((value % 2) == 0) value /= 2;
        else value = -((value+1)/2);


        //if(blockcounter == 52) cout << predictor << " ";


        original = decodeSingleResidual(value, predictor, lastvals, blockcounter == 52);
        lastvals[3] = lastvals[2];
        lastvals[2] = lastvals[1];
        lastvals[1] = lastvals[0];
        lastvals[0] = original;



        output.push_back(original);
    }
    return output;
}


void RealAudioCodec::writeHeader(ofstream& outfile, long samples, int bs, int channels, int fact, int lf) {
    outfile.write((char*)&samples, sizeof(long));
    outfile.write((char*)&bs, sizeof(int));
    char ch = (char) (channels & 0xFF);
    outfile.write(&ch, sizeof(char));
    ch = (char) (fact & 0xFF);
    outfile.write(&ch, sizeof(char));
    outfile.write((char*)&lf, sizeof(int));
}


void RealAudioCodec::readHeader(ifstream& infile, long *samples, int *bs, int *channels, int *fact, unsigned int *lf) {
    infile.read((char*)samples, sizeof(long));
    infile.read((char*)bs, sizeof(int));
    char tmp;
    infile.read(&tmp, sizeof(char));
    *channels = (int) tmp;
    infile.read(&tmp, sizeof(char));
    *fact = (int) tmp;
    infile.read((char*)lf, sizeof(int));
}


void RealAudioCodec::replenish(ifstream& infile) {
    char tmp;
    infile.read(&tmp, sizeof(char));
    buffer = (buffer << 8) | (tmp & 0xFF);
    bitsinbuffer += 8;
}

char RealAudioCodec::readNextByte(ifstream& infile) {
    if (bitsinbuffer < 8) replenish(infile);
    char val = (char) ((buffer >> (bitsinbuffer - 8)) & 0xFF);
    bitsinbuffer -= 8;
    buffer = buffer & buildOnes(bitsinbuffer);
    return val;
}

int RealAudioCodec::readNextPredictor(ifstream& infile) {
    if (bitsinbuffer < 2) replenish(infile);
    int val = (buffer >> (bitsinbuffer - 2)) & 0x03;
    bitsinbuffer -= 2;
    buffer = buffer & buildOnes(bitsinbuffer);
    return val;
}

int RealAudioCodec::readNextQ(ifstream& infile) {
    bool decided = false;
    int counter = 0, bitval;
    while(!decided) {
        if (bitsinbuffer < 1) replenish(infile);
        bitval = ((buffer >> (bitsinbuffer - 1)) & 1);
        bitsinbuffer--;
        buffer = buffer & buildOnes(bitsinbuffer);
        if(bitval == 1) counter++;
        else decided = true;
    }
    return counter;
}


int RealAudioCodec::readNextR(ifstream& infile, int nobits) {
    while (bitsinbuffer < nobits) replenish(infile);
    unsigned int val = (buffer >> (bitsinbuffer - nobits)) & buildOnes(nobits);
    bitsinbuffer -= nobits;
    buffer = buffer & buildOnes(bitsinbuffer);
    return val;
}



int RealAudioCodec::decodeSingleResidual(int input, int winner, int *last_vals, bool debug) {
    int nextval;
    int order = winner + 1;
    //cout << "Using order: " << order << endl;
    if (order == 1) {
        nextval = (input + last_vals[0]);
    }
    else if (order == 2) {
        nextval = (input + 2 * last_vals[0] - last_vals[1]);
    }
    else if (order == 3) {
        nextval = (input + 3 * last_vals[0] - 3 * last_vals[1] + last_vals[2]);
    }
    else if (order == 4) {
        nextval = (input + 4 * last_vals[0] - 6 * last_vals[1] + 4 * last_vals[2] - last_vals[3]);

    }
    /*if(debug) cout << "Order was " << order << ", input " << input << " lastvals: [" << last_vals[0] << " " << last_vals[1]
                   << " "<< last_vals[2] << " "<< last_vals[3] << "] result: " << nextval
                   << " alternatives: " << (input + last_vals[0]) << " " << (input + 2 * last_vals[1] - last_vals[0])
                   << " " << (input + 3 * last_vals[2] - 3 * last_vals[1] + last_vals[0]) << " "
                   << " " << (input + 4 * last_vals[3] - 6 * last_vals[2] + 4 * last_vals[1] - last_vals[0])
                   << endl;*/
    return nextval;
}


int RealAudioCodec::buildOnes(int n) {
    unsigned int tmp = ~0;
    return (tmp >> (32-n));
}

int RealAudioCodec::decodeToWav(ifstream& input, vector<short>& outLEFT, vector<int>& outDELTA) {
    buffer = 0;
    bitsinbuffer = 0;
    long total_samples;
    int bs, channels, fact;
    unsigned int lf;
    readHeader(input, &total_samples, &bs, &channels, &fact, &lf);
    cout << "\nHeader:\nSamples: " << total_samples << "\nBlock size: " << bs << "\nChannels: " << channels << "\nPartition factor: " << fact << endl;
    if(lf != 0) {
        cout << "Reconstruction levels: " << ((SHRT_MAX - SHRT_MIN) / lf) << endl;
    }
    cout << "\nDecoding file...\n";
    vector<int> original_input = decodeGolombFromFile(input, total_samples, bs, fact);


    int value;
    if(lf != 0) {
        double delta = (SHRT_MAX - SHRT_MIN) / ((SHRT_MAX - SHRT_MIN) / lf);
        if (channels == 2) {
            for (int i = 0; i < original_input.size(); i++) {
                value = (int) (original_input.at(i) * delta);
                if (i < original_input.size() / 2) outLEFT.push_back((short) value);
                else outDELTA.push_back(value);

            }
        } else {
            for (int i = 0; i < original_input.size(); i++) {
                value = (int) (original_input.at(i) * delta);
                outLEFT.push_back((short) value);
            }
        }
    } else {
        if (channels == 2) {
            for (int i = 0; i < original_input.size(); i++) {
                value = (int) original_input.at(i);
                if (i < original_input.size() / 2) outLEFT.push_back((short) value);
                else outDELTA.push_back(value);

            }
        } else {
            for (int i = 0; i < original_input.size(); i++) {
                value = (short) original_input.at(i);
                outLEFT.push_back(value);
            }
        }
    }
    return channels;
}

vector<short> RealAudioCodec::lossyRecover(vector<short>& in) {
    vector<short> out;
    short actual;
    short next;
    int mean;
    for(int i = 0; i < (in.size() - 1); i++) {
        actual = in.at(i);
        next = in.at(i+1);
        mean = ((int) actual + (int) next)/2;
        out.push_back(actual);
        out.push_back((short) mean);
    }
    out.push_back(in.at(in.size()-1));
    return out;
}

vector<int> RealAudioCodec::lossyRecover(vector<int>& in) {
    vector<int> out;
    int actual;
    int next;
    int mean;
    for(int i = 0; i < (in.size() - 1); i++) {
        actual = in.at(i);
        next = in.at(i+1);
        mean = (actual + next)/2;
        out.push_back(actual);
        out.push_back(mean);
    }
    out.push_back(in.at(in.size()-1));
    return out;
}
