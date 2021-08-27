#define OPEN_FILE_BUTTON 1
#define EXIT 2

#ifndef UNICODE
#define UNICODE
#endif

#include <windows.h>
#include <iostream>
#include <fstream>
#include <string>
#include <math.h>
#include <map>
#include <bits/stdc++.h>

// #include <itpp/srccode/lpcfunc.h>
// #include <arcd.h>
using namespace std;
// using namespace itpp;

// store 16bit wave data
struct WavData
{
public:
    int16_t *dataL;
    int16_t *dataR;
    long Size;
    DWORD sampleRate;

    WavData()
    {
        dataL = NULL;
        dataR = NULL;
        Size = 0;
        sampleRate = 0;
    }
};

// build tree
// reference from https://www.geeksforgeeks.org/implement-min-heap-using-stl/
// reference from https://www.geeksforgeeks.org/huffman-decoding/
class Node
{
public:
    string hexData;
    int frequency;
    Node *L, *R;
    Node(string data, int n)
    {
        hexData = data;
        frequency = n;
        L = R = NULL;
    }
};

struct myCompare
{
    bool operator()(Node *p1, Node *p2)
    {
        return (p1->frequency > p2->frequency);
    }
};
priority_queue<Node *, vector<Node *>, myCompare> tree;

// store the frequency
map<string, int> freq;

// store final huffman value
map<string, string> codes;

// store # bytes used for original file
int bytesOld = 0;

//GUI
LRESULT CALLBACK WindowProcedure(HWND, UINT, WPARAM, LPARAM);
char *wchar_to_char(const wchar_t *);
void open_file(HWND);

//.wav operation
void loadWav(const char *);

// prepare for encoding
void calLpc(WavData &, short);
int16_t *linerPredic(int16_t *, long);
void coupling(WavData &);

// encoding
void HM(WavData &);
void calFreq(string);
void assignCode(struct Node *, string);
void HuffmanCodes(int);
void getResult(string);

// save res to file
void saveres2file(string &, const char *);
// calcuate compression ratio
void calCompression();
// write to files
void saveByte2file(WavData &, const char *);
void saveOrigin(WavData &, const char *);
// GUI: refer to https://www.youtube.com/watch?v=8GCvZs55mEM&list=PLWzp0Bbyy_3i750dsUj7yq4JrPOIUR_NK
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrerInst, LPSTR args, int ncmdshow)
{

    WNDCLASS wc = {};
    wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hInstance = hInst;
    wc.lpszClassName = L"myWindowClass";
    wc.lpfnWndProc = WindowProcedure;

    if (!RegisterClassW(&wc))
    {
        return -1;
    }

    HWND hDlg = CreateWindowW(L"myWindowClass", L"My Window", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 100, 100, 500, 500, NULL, NULL, NULL, NULL);
    CreateWindowW(L"Button", L"Open File", WS_VISIBLE | WS_CHILD, 10, 100, 150, 36, hDlg, (HMENU)OPEN_FILE_BUTTON, NULL, NULL);
    CreateWindowW(L"Button", L"EXIT", WS_VISIBLE | WS_CHILD, 200, 100, 150, 36, hDlg, (HMENU)EXIT, NULL, NULL);

    MSG msg = {0};
    while (GetMessage(&msg, NULL, NULL, NULL))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

// inspired by https://stackoverflow.com/questions/3019977/convert-wchar-t-to-char
char *wchar_to_char(const wchar_t *pwchar)
{
    int currIdx = 0;
    char currChar = pwchar[currIdx];

    while (currChar != '\0')
        currChar = pwchar[++currIdx];

    // allocate char *
    char *res = (char *)malloc(sizeof(char) * (++currIdx));
    res[currIdx] = '\0';
    for (int i = 0; i < currIdx; i++)
        res[i] = char(pwchar[i]);

    return res;
}

// working procedure for GUI
LRESULT CALLBACK WindowProcedure(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
    case WM_COMMAND:
        switch (wp)
        {
        case OPEN_FILE_BUTTON:
            open_file(hWnd);
            break;
        case EXIT:
            DestroyWindow(hWnd);
            break;
        }
        break;
    case WM_DESTROY:
        exit(0);
        break;
    default:
        return DefWindowProcW(hWnd, msg, wp, lp);
    }
    return 0;
}

// open file dialog (need to connect lib libcomdlg32.a)
void open_file(HWND hWnd)
{
    // define structure

    OPENFILENAME ofn;

    wchar_t file_name[100];
    ZeroMemory(&ofn, sizeof(OPENFILENAME));

    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = hWnd;
    ofn.lpstrFile = file_name;
    ofn.lpstrFile[0] = '\0';
    ofn.nMaxFile = 500;
    ofn.lpstrFilter = L"All files\0*.*\0Audio Files\0*.wav\0";
    ofn.nFilterIndex = 1;

    // should link to lib: libcomdlg32.a
    GetOpenFileName(&ofn);
    char *converted;
    converted = wchar_to_char(ofn.lpstrFile);

    loadWav(static_cast<const char *>(converted));
}

// reference: http://soundfile.sapp.org/doc/WaveFormat/
void loadWav(const char *wav)
{
    // only work for 16bit mono or stereo wav

    FILE *fp = fopen(wav, "rb");

    if (!fp)
        cout << "Error: Failed to open FILE" << endl;

    char type[5];
    DWORD _size, chunkSize;
    short formatType, channels;
    DWORD sampleRate, avgBytesPerSec;
    short bytesPerSample, bitsPerSample;
    DWORD dataSize;

    // read wav header file

    fread(type, sizeof(char), 4, fp);
    type[4] = '\0';

    if (strcmp(type, "RIFF"))

        cout << "Error: Not  RIFF format!" << endl;

    fread(&_size, sizeof(DWORD), 1, fp);
    fread(type, sizeof(char), 4, fp);
    type[4] = '\0';

    if (strcmp(type, "WAVE"))
        cout << "Error: Not Wav" << endl;

    fread(type, sizeof(char), 4, fp);
    type[4] = '\0';

    if (strcmp(type, "fmt "))
        cout << "Error: not fmt" << endl;

    fread(&chunkSize, sizeof(DWORD), 1, fp);
    fread(&formatType, sizeof(short), 1, fp);
    fread(&channels, sizeof(short), 1, fp);
    fread(&sampleRate, sizeof(DWORD), 1, fp);
    fread(&avgBytesPerSec, sizeof(DWORD), 1, fp);
    fread(&bytesPerSample, sizeof(short), 1, fp);
    fread(&bitsPerSample, sizeof(short), 1, fp);
    fread(type, sizeof(char), 4, fp);
    type[4] = '\0';

    if (strcmp(type, "data"))
        cout << "Error: Missing Data" << endl;

    fread(&dataSize, sizeof(DWORD), 1, fp);
    cout << "Chunk size : " << chunkSize << endl;
    cout << "Format Type : " << formatType << endl;
    cout << "Channels : " << channels << endl;
    cout << "SampleRate : " << sampleRate << endl;
    cout << "Average Byte per Sec : " << avgBytesPerSec << endl;
    cout << "Bytes per sample : " << bytesPerSample << endl;
    cout << "Bit per sample : " << bitsPerSample << endl;
    cout << "Total data size: " << dataSize << endl;

    WavData ret;
    ret.Size = dataSize / bytesPerSample;
    cout << "Total numbe of sample for each channel: " << ret.Size << endl;
    ret.sampleRate = sampleRate;
    bytesOld = ret.Size * bitsPerSample * channels;
    ret.dataL = new int16_t[ret.Size];
    if (channels == 2)
    {
        ret.dataR = new int16_t[ret.Size];
        int16_t *tempL = ret.dataL;
        int16_t *tempR = ret.dataR;
        for (int i = 0; i < ret.Size; i++)
        {
            fread(tempL, bytesPerSample / channels, 1, fp);
            fread(tempR, bytesPerSample / channels, 1, fp);
            tempR++;
            tempL++;
        }
    }
    else
        fread(ret.dataL, bytesPerSample / channels, ret.Size, fp);

    // saveByte2file(ret, "origin.txt");
    saveOrigin(ret, "old.txt");

    // start encoding
    // LPC
    calLpc(ret, channels);
    // channel coupling
    if (channels == 2)
        coupling(ret);
    saveByte2file(ret, "clean.txt");
    cout << "Start Encoding: " << endl;
    // encoder
    HM(ret);
    cout << "End Encoding. " << endl;
}

void saveByte2file(WavData &ret, const char *file)
{
    ofstream outL(file);
    if (ret.dataR != NULL)
        for (long i = 0; i < ret.Size; i++)
            outL << hex << ret.dataL[i] << ' ' << ret.dataR[i];
    else
        for (long i = 0; i < ret.Size; i++)
            outL << hex << ret.dataL[i] << ' ';

    outL.close();
}

void saveOrigin(WavData &ret, const char *file)
{
    ofstream outL(file);
    if (ret.dataR != NULL)
        for (long i = 0; i < ret.Size; i++)
            outL << hex << ret.dataL[i] << ret.dataR[i];
    else
        for (long i = 0; i < ret.Size; i++)
            outL << hex << ret.dataL[i];

    outL.close();
}

void calLpc(WavData &ret, short channels)
{
    ret.dataL = linerPredic(ret.dataL, ret.Size);
    if (channels == 2)
        ret.dataR = linerPredic(ret.dataR, ret.Size);
}

int16_t *linerPredic(int16_t *data, long dataSize)
{
    /*vec dataChannel(dataSize);
    int16_t *prediced = new int16_t[dataSize];
    for (int idx = 0; idx < dataSize; idx++)
        dataChannel[idx] = data[idx];
    int order = 1;
    double coe;
    coe = lpc(dataChannel, order)[1] * -1;
    prediced[0] = dataChannel[0];
    for (int idx = 1; idx < dataSize; idx++)
        prediced[idx] = int16_t(coe * dataChannel[idx - 1]);

    return prediced;*/

    // can simply store the difference between the previous and current values
    int16_t *newData = new int16_t[dataSize];
    newData[0] = data[0];
    for (int idx = 1; idx < dataSize; idx++)
        newData[idx] = data[idx] - data[idx - 1];

    return newData;
}

void coupling(WavData &ret)
{
    int16_t mid;
    int16_t side;
    for (int idx = 0; idx < ret.Size; idx++)
    {
        mid = (ret.dataL[idx] + ret.dataR[idx]) / 2;
        side = (ret.dataL[idx] - ret.dataR[idx]) / 2;
        ret.dataL[idx] = mid;
        ret.dataR[idx] = side;
    }
}

void calFreq(string line)
{
    string temp1 = "", temp2 = "";
    for (int i = 0; i < line.length(); i++)
    {
        while (i < line.length() && line[i] != ' ')
        {
            if (temp1.length() < 2)
                temp1 += line[i];
            else
                temp2 += line[i];
            i++;
        }
        //cout << temp1 << " " << temp2 << endl;
        if (temp1 != "")
            freq[temp1]++;
        if (temp2 != "")
            freq[temp2]++;
        temp1 = "";
        temp2 = "";
    }
}

void assignCode(struct Node *top, string codeword)
{
    if (top != NULL)
    {
        if (top->hexData != "$")
            codes[top->hexData] = codeword;
        assignCode(top->L, codeword + '0');
        assignCode(top->R, codeword + '1');
    }
}

void HuffmanCodes(int SIZE)
{
    for (auto v = freq.begin(); v != freq.end(); v++)
        tree.push(new Node(v->first, v->second));
    Node *L, *R, *M;
    while (tree.size() != 1)
    {
        L = tree.top();
        tree.pop();
        R = tree.top();
        tree.pop();
        M = new Node("$", L->frequency + R->frequency);
        M->L = L;
        M->R = R;
        tree.push(M);
    }
    assignCode(tree.top(), "");
}

void saveres2file(string &res, const char *file)
{
    ofstream outL(file);
    for (long i = 0; i < res.length(); i++)
        outL << res[i];

    outL.close();
}

void calCompression()
{
    cout << "Before encoding, the bytes used to store samples are : " << round(bytesOld / 8.0) << endl;
    int bit = 0;
    for (auto v = freq.begin(); v != freq.end(); v++)
        bit += v->second * codes[v->first].length();
    float bytes = bit / 8.0;
    cout << "After encoding, the bytes used to store samples are : " << round(bytes) << endl;
    cout << "Compression Ratio is " << (bytesOld / 8.0) / bytes << endl;

    // clear Huffman tree
    freq.clear();
    codes.clear();
    while (!tree.empty())
        tree.pop();
}

// reference to geeksforgeeks
void createMap(unordered_map<string, char> *um)
{
    (*um)["0000"] = '0';
    (*um)["0001"] = '1';
    (*um)["0010"] = '2';
    (*um)["0011"] = '3';
    (*um)["0100"] = '4';
    (*um)["0101"] = '5';
    (*um)["0110"] = '6';
    (*um)["0111"] = '7';
    (*um)["1000"] = '8';
    (*um)["1001"] = '9';
    (*um)["1010"] = 'A';
    (*um)["1011"] = 'B';
    (*um)["1100"] = 'C';
    (*um)["1101"] = 'D';
    (*um)["1110"] = 'E';
    (*um)["1111"] = 'F';
}

// function to find hexadecimal
// equivalent of binary
string convertBinToHex(string bin)
{
    int l = bin.size();
    int t = bin.find_first_of('.');

    // length of string before '.'
    int len_left = t != -1 ? t : l;

    // add min 0's in the beginning to make
    // left substring length divisible by 4
    for (int i = 1; i <= (4 - len_left % 4) % 4; i++)
        bin = '0' + bin;

    // if decimal point exists
    if (t != -1)
    {
        // length of string after '.'
        int len_right = l - len_left - 1;

        // add min 0's in the end to make right
        // substring length divisible by 4
        for (int i = 1; i <= (4 - len_right % 4) % 4; i++)
            bin = bin + '0';
    }

    // create map between binary and its
    // equivalent hex code
    unordered_map<string, char> bin_hex_map;
    createMap(&bin_hex_map);

    int i = 0;
    string hex = "";

    while (1)
    {
        // one by one extract from left, substring
        // of size 4 and add its hex code
        hex += bin_hex_map[bin.substr(i, 4)];
        i += 4;
        if (i == bin.size())
            break;

        // if '.' is encountered add it
        // to result
        if (bin.at(i) == '.')
        {
            hex += '.';
            i++;
        }
    }

    // required hexadecimal number
    return hex;
}
void getResult(string line)
{
    string temp1 = "", temp2 = "", r = "";
    for (int i = 0; i < line.length(); i++)
    {
        while (i < line.length() && line[i] != ' ')
        {
            if (temp1.length() < 2)
                temp1 += line[i++];
            else
                temp2 += line[i++];
        }
        if (temp1 != "")
            r += codes[temp1];
        if (temp2 != "")
            r += codes[temp2];

        temp1 = "";
        temp2 = "";
    }
    r = convertBinToHex(r);
    saveres2file(r, "result.txt");
    calCompression();
}

void HM(WavData &ret)
{
    string byteArray;
    ifstream myfile("clean.txt");
    if (myfile.is_open())
    {
        getline(myfile, byteArray);
    }

    cout << "Calculate Frequency" << endl;
    calFreq(byteArray);

    cout << "HuffmanCoding" << endl;
    HuffmanCodes(byteArray.length());

    cout << "Store Result" << endl;
    getResult(byteArray);
}
