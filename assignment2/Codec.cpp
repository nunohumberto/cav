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
            ("l,lossy", "Applies lossy compression", cxxopts::value<int>(), "<symbol space divisions>")
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


    AudioEntropy ae;
    bool lossy = result.count("lossy") != 0;
    int lossy_factor = 0;
    if (lossy) {
        lossy_factor = result["lossy"].as<int>();
        ae.setupQuantizer(lossy_factor);
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
            //if(result.count("lossy")) {
            //    outLEFT = codec.lossyRecover(outLEFT);
            //    outDELTA = codec.lossyRecover(outDELTA);
            //}
            codec.writeWavToFile(result["decode"].as<string>(), outLEFT, outDELTA, channels);
        }
        else {
            RealAudioCodec codec;
            channels = codec.decodeToWav(infile, outLEFT, outDELTA);
            //if(result.count("lossy")) {
            //    outLEFT = codec.lossyRecover(outLEFT);
            //    outDELTA = codec.lossyRecover(outDELTA);
            //}
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
    vector<int> residual[4];



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
        if (!lossy) {
            if(channels == 1) sample[0] = buff[i];
            else {
                sample[0] = buff[2*i];
                sample[1] = buff[2*i + 1];
            }
        } else {
            if(channels == 1) sample[0] = ae.quantize(buff[i]);
            else {
                sample[0] = ae.quantize(buff[2*i]);
                sample[1] = ae.quantize(buff[2*i + 1]);
            }
        }


        if(draw) {
            if (sndmapL.count(sample[0]) == 0) sndmapL[sample[0]] = 0;
            if (sndmapALL.count(sample[0]) == 0) sndmapALL[sample[0]] = 0;
            sndmapL[sample[0]] = sndmapL[sample[0]] + 1;
        }
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


    }
    delete[] buff;
    vecALLappend.insert(vecALLappend.end(), vecL.begin(), vecL.end());
    vecALLappend.insert(vecALLappend.end(), vecDelta.begin(), vecDelta.end());
    vecL.clear();
    vecR.clear();
    vecAVG.clear();
    vecDelta.clear();
    vecALL.clear();


    short test = -1;

    int *counts = (int*) calloc(2*SHRT_MAX - 2*SHRT_MIN, sizeof(int));
    ae.arrFromIntVector(vecALLappend, counts);
    double entropy = ae.calcEntropy(counts);
    delete counts;

    cout << "Input entropy: " << entropy << endl;
    cout << "(Stage \033[1;34m2\033[0m/\033[1;31m" << total_stages << "\033[0m) Calculating residuals..." << endl;
    common.calculateResiduals(vecALLappend, residual);
    int bs = 512;
    //map<int, int> comparison = residualStats(residual, bs);
    //cerr << "Done calculating stats.\n";
    vector<int> lowest_values;
    vector<int> lowest_value_predictors = common.residualComparison(residual, bs, lowest_values);

    //vector<int> lowest_value_predictors = residualComparison(residual, vecALLappend, bs, lowest_values);
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



    counts = (int*) calloc(2*SHRT_MAX - 2*SHRT_MIN, sizeof(int));
    //m = ae.mapFromIntVector(lowest_values);
    ae.arrFromIntVector(lowest_values, counts);
    //cout << "Residuals entropy: " << ae.calcEntropy(m) << endl;
    cout << "Residuals entropy: " << ae.calcEntropy(counts) << endl;
    delete counts;
    //int lowest = getResiduesWithLowestEntropy(residual, ae); UNCOMMENT FOR REPORT
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

    vector<string> fake_golomb_encoded;

    long bestsize = 0;
    long totalsize = 0;
    long max_slice;
    int best_m;
    vector<int> partition;


    long totalsamples = 0;
    long partition_size = (bs*2);
    int number_of_partitions = (int) ceil((lowest_values.size()*1.0)/partition_size);

    int fact = 0;
    cout << "(Stage \033[1;34m" << total_stages-1 << "\033[0m/\033[1;31m" << total_stages << "\033[0m) Discovering best number of partitions... " << endl;
    vector<int> m_values = common.findBestPartitionNumber(lowest_values, bs, &totalsize, &fact);
    partition_size = (bs*fact);
    number_of_partitions = (int) ceil((lowest_values.size()*1.0)/partition_size);
    cout << "Expected size with " << number_of_partitions << " partitions: " << totalsize/8 << " bytes." << endl;



    if(result.count("fake")) {
        FakeAudioCodec codec;
        cout << "(Stage \033[1;34m" << total_stages << "\033[0m/\033[1;31m" << total_stages << "\033[0m) Writing textual file to: " << encoded_filename << endl;
        ofstream outfile(encoded_filename);
        codec.fakeWriteHeader(outfile, lowest_values.size(), bs, channels, fact, lossy_factor);
        codec.fakeEncodeGolombToFile(lowest_values, lowest_value_predictors, m_values, outfile, bs, fact);
        outfile.close();
    }
    else {
        RealAudioCodec codec;
        cout << "(Stage \033[1;34m" << total_stages << "\033[0m/\033[1;31m" << total_stages << "\033[0m) Writing encoded file to: " << encoded_filename << endl;
        ofstream outfile(encoded_filename, ios::out | ios::binary);
        codec.writeHeader(outfile, lowest_values.size(), bs, channels, fact, lossy_factor);
        codec.encodeGolombToFile(lowest_values, lowest_value_predictors, m_values, outfile, bs, fact);
        outfile.close();
    }


    chrono::milliseconds now = chrono::duration_cast< chrono::milliseconds >(chrono::system_clock::now().time_since_epoch());
    long elapsed = (now-before).count();
    cout << "\033[1;32mDone. Time elapsed: " << elapsed/1000.0 << " seconds.\033[0m" << endl;

    if(draw) cvWaitKey(0);


}




