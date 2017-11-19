//
// Created by Nuno Humberto on 19/11/2017.
//

#include "FakeAudioCodec.h"

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


void FakeAudioCodec::writeWavToFile(string filename, vector<short>& left, vector<int>& delta, int chan) {
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

vector<int> FakeAudioCodec::fakeDecodeGolombFromFile(ifstream& input, long total_samples, int bs, int factor) {
    vector<int> output;
    int blockcounter = 0;
    int m, nbits, predictor, q, r, original;
    int value;
    int lastvals[4] = {0};
    string buffer;
    for(long readsamples = 0; readsamples < total_samples; readsamples++) {
        if ((readsamples % bs) == 0) {
            if ((blockcounter % factor) == 0) {
                getline(input, buffer);
                nbits = stoi(buffer);

                m = pow(2, nbits);
            }
            getline(input, buffer);
            predictor = stoi(buffer);
            blockcounter++;
        }
        getline(input, buffer);
        q = stoi(buffer);

        getline(input, buffer);
        r = stoi(buffer);

        value = m*q + r;



        if((value % 2) == 0) value /= 2;
        else value = -((value+1)/2);


        //if(blockcounter == 52) cout << predictor << " ";


        original = decodeSingleResidue(value, predictor, lastvals, blockcounter == 52);
        lastvals[3] = lastvals[2];
        lastvals[2] = lastvals[1];
        lastvals[1] = lastvals[0];
        lastvals[0] = original;



        output.push_back(original);
    }
    return output;
}

void FakeAudioCodec::fakeWriteHeader(ofstream& outfile, long samples, int bs, int channels, int fact) {
    outfile << samples;
    outfile << '\n';
    outfile << bs;
    outfile << '\n';
    outfile << channels;
    outfile << '\n';
    outfile << fact;
    outfile << '\n';
}

void FakeAudioCodec::fakeReadHeader(ifstream& infile, long *samples, int *bs, int *channels, int *fact) {
    string buffer;
    getline(infile, buffer);
    *samples = stol(buffer);
    getline(infile, buffer);
    *bs = stoi(buffer);
    getline(infile, buffer);
    *channels = stoi(buffer);
    getline(infile, buffer);
    *fact = stoi(buffer);
}


void FakeAudioCodec::fakeEncodeGolombToFile(vector<int>& input, vector<int>& predictors, vector<int>& m_vector, ofstream& outfile, int bs, int factor) {
    vector<int>::iterator it;
    int target;
    int q, r;
    int nbits;// = (int) log2(m);
    int m;
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

                outfile << nbits << '\n';
            }

            blockcounter++;

            outfile << predictors.at(i/bs) << '\n';
            //estsize += 2;
        }



        q = target/m;

        outfile << q << '\n';
        //estsize += (q+1);

        r = target - q * m;

        //if(blockcounter == 52) cout << predictors.at(i/bs) << " ";

        outfile << r << '\n';

        //estsize += nbits;
    }
    //cout << endl;

    //cout << "Wrote a total of " << partition_index << " partitions.\n";
    //cout << "Estimated size: " << estsize/8 << endl;
}



int FakeAudioCodec::decodeSingleResidue(int input, int winner, int last_vals[], bool debug) {
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

int FakeAudioCodec::fakeDecodeToWav(ifstream& input, vector<short>& outLEFT, vector<int>& outDELTA) {
    long total_samples;
    int bs, channels, fact;
    fakeReadHeader(input, &total_samples, &bs, &channels, &fact);
    cout << "\nHeader:\nSamples: " << total_samples << "\nBlock size: " << bs << "\nChannels: " << channels << "\nPartition factor: " << fact << endl;

    cout << "\nDecoding file...\n";
    vector<int> original_input = fakeDecodeGolombFromFile(input, total_samples, bs, fact);
    int value;
    if(channels == 2) {
        for (int i = 0; i < original_input.size(); i++) {
            value = (short) original_input.at(i);
            if (i < original_input.size() / 2) outLEFT.push_back((short) value);
            else outDELTA.push_back(value);

        }
    } else {
        for (int i = 0; i < original_input.size(); i++) {
            value = (short) original_input.at(i);
            outLEFT.push_back(value);
        }
    }
    return channels;
}

vector<short> FakeAudioCodec::lossyRecover(vector<short>& in) {
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

vector<int> FakeAudioCodec::lossyRecover(vector<int>& in) {
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
