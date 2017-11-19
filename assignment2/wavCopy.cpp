#include <stdio.h>
#include <stdlib.h>
#include <map>
#include <sndfile.hh>
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <iostream>
#include <fstream>
#include "cxxopts.hpp"
#include <chrono>
#include <vector>
#include <limits>
#include "AudioEntropy.h"
#include "Common.h"
#include "FakeAudioCodec.h"
#include "RealAudioCodec.h"

using namespace std;


int main(int argc, char **argv) {
    int tmpargc = argc;

    cxxopts::Options options(argv[0], "CAV Lossless Audio Codec\nDeveloped by Nuno Humberto and Nuno Miguel Silva\n");
    options.add_options()
            ("i,input", "Input file", cxxopts::value<std::string>(), "<input file>")
            ("d,decode", "Specifies a decode operation", cxxopts::value<std::string>(), "<output file>")
            ("e,encode", "Specifies an encode operation", cxxopts::value<std::string>(), "<output file>")
            ("draw", "Draws histograms")
            ("l,lossy", "Applies lossy compression/decompression")
            ("f,fake", "Generates/decodes textual files")
            ("help", "Prints help");

    try {
        auto result = options.parse(argc, argv);

    } catch (const cxxopts::OptionException& e) {
        std::cout << "error parsing options: " << e.what() << std::endl;
        return 1;
    }
    argc = tmpargc;


    auto result = options.parse(argc, argv);

    if (result.count("help")) {
        cout << options.help({""}) << endl;
        return 0;
    }

    if (result.count("decode") && result.count("input"))
    {
        chrono::milliseconds before = chrono::duration_cast< chrono::milliseconds >(chrono::system_clock::now().time_since_epoch());
        ifstream infile(result["input"].as<string>(), ios::in | ios::binary);

        vector<short> outLEFT;
        vector<int> outDELTA;

        cout << "Loading file: " << result["input"].as<string>() << "...\n";

        int channels;
        if(result.count("fake")) {
            FakeAudioCodec codec;
            channels = codec.fakeDecodeToWav(infile, outLEFT, outDELTA);
            if(result.count("lossy")) {
                outLEFT = codec.lossyRecover(outLEFT);
                outDELTA = codec.lossyRecover(outDELTA);
            }
            codec.writeWavToFile(result["decode"].as<string>(), outLEFT, outDELTA, channels);
        }
        else {
            RealAudioCodec codec;
            channels = codec.decodeToWav(infile, outLEFT, outDELTA);
            if(result.count("lossy")) {
                outLEFT = codec.lossyRecover(outLEFT);
                outDELTA = codec.lossyRecover(outDELTA);
            }
            codec.writeWavToFile(result["decode"].as<string>(), outLEFT, outDELTA, channels);
        }

        chrono::milliseconds now = chrono::duration_cast< chrono::milliseconds >(chrono::system_clock::now().time_since_epoch());
        chrono::milliseconds elapsed = now-before;
        cout << "\033[1;32mDone. Time elapsed: " << elapsed.count()/1000.0 << " seconds.\033[0m" << endl;
        return 0;
    }

    else if (!(result.count("encode") && result.count("input"))) {
        cout << options.help({""}) << endl;
        return 1;
    }

    bool draw = false;
    int total_stages = 4;
    if (result.count("draw")) {
        draw = true;
        total_stages++;
    }
    string encoded_filename = result["encode"].as<string>();
    SndfileHandle soundFileIn;

    chrono::milliseconds before = chrono::duration_cast< chrono::milliseconds >(chrono::system_clock::now().time_since_epoch());

    int i;
    short *sample = new short[2];
    sf_count_t nSamples = 2;

    map<short, int> sndmapL;
    map<short, int> sndmapR;
    map<short, int> sndmapALL;
    map<short, int> sndmapAVG;
    vector<short> vecL;
    vector<short> vecR;
    vector<short> vecALL;
    vector<int> vecALLappend;
    vector<int> vecDelta;
    vector<int> vecAVG;
    vector<int> residues[4];


    AudioEntropy ae;
    Common common;
    string input_file = result["input"].as<string>();

    soundFileIn = SndfileHandle(input_file);
    cout << "Detected file header:\n";
    fprintf(stderr, "Frames (samples): %d\n", (int) soundFileIn.frames());
    fprintf(stderr, "Samplerate: %d\n", soundFileIn.samplerate());
    fprintf(stderr, "Channels: %d\n", soundFileIn.channels());

    short *buff = new short[soundFileIn.frames()*soundFileIn.channels()];
    soundFileIn.readRaw(buff, soundFileIn.frames()*soundFileIn.channels()* sizeof(short));
    cout << "(Stage \033[1;34m1\033[0m/\033[1;31m" << total_stages << "\033[0m) Loading file...\n";
    int channels = soundFileIn.channels();
    int srate = soundFileIn.samplerate();

    for (i = 0; i < soundFileIn.frames(); i++) {
        //if (soundFileIn.read(sample, soundFileIn.channels()) == 0) {
        //    fprintf(stderr, "Error: Reached end of file %d\n", i);
        //    break;
        if(channels == 1) sample[0] = buff[i];
        else {
            sample[0] = buff[2*i];
            sample[1] = buff[2*i + 1];
        }

        if(result.count("lossy") && (i % 2 != 0)) continue;




        if (sndmapL.count(sample[0]) == 0) sndmapL[sample[0]] = 0;
        if (draw) if (sndmapALL.count(sample[0]) == 0) sndmapALL[sample[0]] = 0;


        sndmapL[sample[0]] = sndmapL[sample[0]] + 1;
        vecL.push_back(sample[0]);


        if(draw) {
            if (sndmapALL.count(sample[0]) == 0) sndmapALL[sample[0]] = 0;
            sndmapALL[sample[0]] = sndmapALL[sample[0]] + 1;
            //vecALL.push_back(sample[0]);
        }




        if(soundFileIn.channels() == 2) {
            if (draw) {
                int mid_index = (int) floor((sample[0] + sample[1])/2.0);
                if (sndmapR.count(sample[1]) == 0) sndmapR[sample[1]] = 0;
                if (sndmapAVG.count(mid_index) == 0) sndmapAVG[mid_index] = 0;
                if (sndmapALL.count(sample[1]) == 0) sndmapALL[sample[1]] = 0;

                sndmapR[sample[1]] = sndmapR[sample[1]] + 1;
                //vecR.push_back(sample[1]);

                sndmapALL[sample[1]] = sndmapALL[sample[1]] + 1;
                //vecALL.push_back(sample[1]);

                sndmapAVG[mid_index] = sndmapAVG[mid_index] + 1;
                //vecAVG.push_back(mid_index);
            }

            vecDelta.push_back(sample[0] - sample[1]);
        }





        /*if (soundFileOut.write(sample, nSamples) != 2) {
            fprintf(stderr, "Error writing frames to the output:\n");
            return -1;
        }*/
    }
    delete[] buff;
    vecALLappend.insert(vecALLappend.end(), vecL.begin(), vecL.end());
    vecALLappend.insert(vecALLappend.end(), vecDelta.begin(), vecDelta.end());
    vecL.clear();
    vecR.clear();
    vecAVG.clear();
    vecDelta.clear();
    vecALL.clear();



    //vecALLappend.insert(vecALLappend.end(), vecR.begin(), vecR.end());

    //cout << "Original samples: ";
    //for(int i = GLOBAL_MIN; i < GLOBAL_MIN+100; i++) cout << vecALLappend.at(i) << " ";

    //cout << " last: ";
    //cout << vecALLappend.at(vecALLappend.size()-1) << " ";
    //cout << "\nsize: " << vecALLappend.size() << endl;

    //for(int i = 0; i < 10; i++) cout << "Last: " << vecALLappend.at(vecALLappend.size()-(1 + i)) << " ";
    //cout << endl;

    short test = -1;

    map<int, int> m = ae.mapFromIntVector(vecALLappend);

    double entropy = ae.calcEntropy(m);
    cout << "Input entropy: " << entropy << endl;
    cout << "(Stage \033[1;34m2\033[0m/\033[1;31m" << total_stages << "\033[0m) Calculating residues..." << endl;
    common.calculateResidues(vecALLappend, residues);
    int bs = 512;
    //map<int, int> comparison = residueStats(residues, bs);
    //cerr << "Done calculating stats.\n";
    vector<int> lowest_values;
    vector<int> lowest_value_predictors = common.residueComparison(residues, bs, lowest_values);
    //vector<int> lowest_value_predictors = residueComparison(residues, vecALLappend, bs, lowest_values);
    /*cout << "Lowest: ";
    for(int i = 0; i < 10; i++) {
       cout << lowest_values.at(i) << " ";
    }
    cout << endl;*/

    //cout << "Predictor statistics: \n";
    //for(int i = 0; i < 4; i++) {
    //    if (comparison.count(i) == 0) continue;
    //   cout << i << ": " << comparison.at(i) << endl;
    //}

    //cout << "Predictor number (" << lowest_value_predictors.size() << ")" << endl;

    m = ae.mapFromIntVector(lowest_values);
    cout << "Residue entropy: " << ae.calcEntropy(m) << endl;
    //int lowest = getResiduesWithLowestEntropy(residues, ae); UNCOMMENT FOR REPORT
    //lowest = 0;

    if(draw) {
        cout << "(Stage \033[1;34m3\033[0m/\033[1;31m" << total_stages << "\033[0m) Drawing histograms..." << endl;
        map<short, int> res = ae.reducedMapFromIntVector(lowest_values);
        //map<short, int> inp = ae.reducedMapFromIntVector(vecALLappend);
        ae.drawHistogram(sndmapL, "Histogram - L");
        ae.drawHistogram(res, "Residues");
        //ae.drawHistogram(inp, "Input");
        res.clear();
        //inp.clear();
        if(soundFileIn.channels() == 2) {
            ae.drawHistogram(sndmapR, "Histogram - R");
            ae.drawHistogram(sndmapAVG, "Histogram - Average (Mono)");
            ae.drawHistogram(sndmapALL, "Total");
        }
    }

    //ae.drawHistogram(sndmapL, "Histogram - L");

    //if(soundFileIn.channels() == 2) {
    //    ae.drawHistogram(sndmapR, "Histogram - R");
    //    ae.drawHistogram(sndmapAVG, "Histogram - Mono");
    //    ae.drawHistogram(sndmapALL, "total");
    //}


    //for(int i = 0; i < 10; i++) {
    //    cout << residues[lowest].at(i) << " ";
    //}
    //cout << endl;

    //vector<string> golomb_encoded = encodeGolomb(residues[lowest]);
    vector<string> fake_golomb_encoded;

    //for(int i = 2; i < 20; i++) {
    //    golomb_encoded = encodeGolomb(residues[lowest], (int) pow(2, i));
    //}
    long bestsize = 0;

    //int best_m = findBestM(lowest_values, bs, &bestsize);
    //long bestsize = 0;
    long totalsize = 0;
    long max_slice;
    int best_m;
    vector<int> partition;
    /*partition = sliceVector(lowest_values, 0, lowest_values.size()/2);
    best_m = findBestM(partition, bs, &bestsize);
    totalsize += bestsize;
    partition = sliceVector(lowest_values, lowest_values.size()/2, lowest_values.size()-1);
    best_m = findBestM(partition, bs, &bestsize);
    totalsize += bestsize;*/

    /*for(int i = 0; i < lowest_values.size()/(bs*4); i++) {
        max_slice = (i+1)*(lowest_values.size()/(bs*4));
        if (max_slice >= lowest_values.size())  partition = sliceVector(lowest_values, i*(lowest_values.size()/(bs*4)), lowest_values.size() - 1);
        else partition = sliceVector(lowest_values, i*(lowest_values.size()/(bs*4)), max_slice - 1);
        cout << "Slice: " << i*(lowest_values.size()/(bs*4)) << " to " << max_slice - 1 << endl;
        best_m = findBestM(partition, bs, &bestsize);
        totalsize += bestsize;
    }*/
    long totalsamples = 0;
    long partition_size = (bs*2);
    int number_of_partitions = (int) ceil((lowest_values.size()*1.0)/partition_size);

    int fact = 0;
    cout << "(Stage \033[1;34m" << total_stages-1 << "\033[0m/\033[1;31m" << total_stages << "\033[0m) Discovering best number of partitions... " << endl;
    vector<int> m_values = common.findBestPartitionNumber(lowest_values, bs, &totalsize, &fact);
    partition_size = (bs*fact);
    number_of_partitions = (int) ceil((lowest_values.size()*1.0)/partition_size);
    cout << "Expected size with " << number_of_partitions << " partitions: " << totalsize/8 << " bytes." << endl;



    //cout << "Encoded " << lowest_values.size() << " samples. reslow: " << residues[lowest].size() << endl;
    //fake_golomb_encoded = encodeGolomb(residues[lowest], best_m);
    if(result.count("fake")) {
        FakeAudioCodec codec;
        cout << "(Stage \033[1;34m" << total_stages << "\033[0m/\033[1;31m" << total_stages << "\033[0m) Writing textual file to: " << encoded_filename << endl;
        ofstream outfile(encoded_filename);
        codec.fakeWriteHeader(outfile, lowest_values.size(), bs, channels, fact);
        codec.fakeEncodeGolombToFile(lowest_values, lowest_value_predictors, m_values, outfile, bs, fact);
        outfile.close();
    }
    else {
        RealAudioCodec codec;
        cout << "(Stage \033[1;34m" << total_stages << "\033[0m/\033[1;31m" << total_stages << "\033[0m) Writing encoded file to: " << encoded_filename << endl;
        ofstream outfile(encoded_filename, ios::out | ios::binary);
        codec.writeHeader(outfile, lowest_values.size(), bs, channels, fact);
        codec.encodeGolombToFile(lowest_values, lowest_value_predictors, m_values, outfile, bs, fact);
        outfile.close();
    }


    chrono::milliseconds now = chrono::duration_cast< chrono::milliseconds >(chrono::system_clock::now().time_since_epoch());
    long elapsed = (now-before).count();
    cout << "\033[1;32mDone. Time elapsed: " << elapsed/1000.0 << " seconds.\033[0m" << endl;
    //for(int i = 0; i < 10; i++) {
    //    cout << fake_golomb_encoded.at(i) << endl;
    //}

    //vector<short> decoded = decodeResidue(residues[lowest], lowest+1);


    /*cout << "Decoded samples: ";
    for(int i = 0; i < 10; i++) cout << decoded.at(i) << " ";

    cout << " last: ";
    cout << decoded.at(decoded.size()-1) << " ";
    cout << endl;*/

    if(draw) cvWaitKey(0);


}




