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

using namespace std;


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
//GUI
LRESULT CALLBACK WindowProcedure(HWND, UINT, WPARAM, LPARAM);
char *wchar_to_char(const wchar_t *);
void open_file(HWND);

//.wav operation
void loadWav(const char *);
void plotWaveForm(string, short, bool);
void write2File(WavData, string);
void effect(WavData, string);
void apply(int16_t *&, long);

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

void write2File(WavData song, string fileName)
{
    ofstream outL(fileName);
    for (long i = 0; i < song.Size; i++)
        outL << double(i) / song.sampleRate << ' ' << int16_t(song.dataL[i]) << '\n';
    outL.close();
    if (song.dataR != NULL)
    {
        ofstream outR("Right.txt");
        for (long i = 0; i < song.Size; i++)
            outR << double(i) / song.sampleRate << ' ' << int16_t(song.dataR[i]) << '\n';
        outR.close();
    }
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

    string data = "Left.txt";
    write2File(ret, data);
    plotWaveForm(data, channels, 1);
    effect(ret, data);
}

void plotWaveForm(string fileName, short channels, bool flag)
{
    FILE *gp;
    gp = popen("gnuplot", "w");
    if (gp == NULL)
    {
        cout << "error" << endl;
    }
    else
    {
        string title;
        if (flag)
            title = "set title 'Waveform'\n";
        else
            title = "set title 'Fade In/Out'\n";
        string command = "plot '" + fileName + "' using 1:2 with lines\n";
        if (channels == 1)
        {
            fprintf(gp, title.c_str());
            fprintf(gp, "set xlabel 'Time (Second)'\n");
            fprintf(gp, "set ylabel 'Sample value(Voltage)'\n");
            fprintf(gp, command.c_str());
            fprintf(gp, "pause mouse\n");
            pclose(gp);
        }
        else
        {
            if (flag)
                title = "set title 'Waveform(Above: L, Bottom: R)'\n";
            else
                title = "set title 'Fade In/Out(Above: L, Bottom: R)'\n";

            fprintf(gp, "set multiplot\n set size 1,0.5\n");
            fprintf(gp, title.c_str());
            fprintf(gp, "set xlabel 'Time (Second)'\n");
            fprintf(gp, "set ylabel 'Sample value(Voltage)'\n");
            fprintf(gp, "set origin 0,0.5\n");
            fprintf(gp, command.c_str());
            fprintf(gp, "set origin 0,0\n");
            fprintf(gp, "plot 'Right.txt' using 1:2 with lines\n");
            fprintf(gp, "pause mouse\n");
            pclose(gp);
        }
    }
}
void apply(int16_t *&data, long SIZE)
{
    long first = SIZE / 2;
    float db = -20.0, ratio = 0.1;
    float inc = 20.0 / (first);
    for (long i = 0; i < first; i++)
    {
        data[i] *= ratio;
        db += inc;
        ratio = pow(10, db / 20);
    }

    db = 0, ratio = 1;
    for (long i = first; i < SIZE; i++)
    {
        data[i] *= ratio;
        db -= inc;
        ratio = pow(10, db / 20);
    }
}
void effect(WavData song, string name)
{
    apply(song.dataL, song.Size);
    int channel = 1;
    if (song.dataR != NULL)
    {
        apply(song.dataR, song.Size);
        channel = 2;
    }

    write2File(song, name);
    plotWaveForm(name, channel, 0);
}
