#include <iostream>
#include <string>
#include <Windows.h>
#include <chrono>
#include <vector>
#include <algorithm>
#include <cmath>

using namespace std;

int nScreenWidth = 90;
int nScreenHeight = 30;

float fPlayerX = 1.0f;
float fPlayerY = 1.0f;
float fPlayerA = 0.0f; // angle of FOV
float fFOV = 3.14159 / 4.0;

int nMapHeight = 16;
int nMapWidth = 16;

float fDepth = 16.0f; // depth of view

int main()
{
    wchar_t* screen = new wchar_t[nScreenWidth * nScreenHeight]; // set up screen (matrix of characters)

    HANDLE hConsole = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL); // HANDLE/BUFFER ??

    SetConsoleActiveScreenBuffer(hConsole); // attach buffer to console
    DWORD dwBytesWritten = 0;

    wstring map;
    // '#' <- is a wall 
    map += L"################";
    map += L"#..............#";
    map += L"#.........#....#";
    map += L"#.........#....#";
    map += L"#.........#....#";
    map += L"#######...######";
    map += L"#..............#";
    map += L"#.......###....#";
    map += L"#..............#";
    map += L"#...##.........#";
    map += L"#..............#";
    map += L"#..............#";
    map += L"#.......####...#";
    map += L"#..............#";
    map += L"#..............#";
    map += L"################";

    auto tp1 = chrono::system_clock::now();
    auto tp2 = chrono::system_clock::now();

    // game loop
    while (true)
    {
        // calculate (millis) time for one frame to make movement stable
        tp2 = chrono::system_clock::now();
        chrono::duration<float> elapsedTime = tp2 - tp1;
        tp1 = tp2;
        float fElapsedTime = elapsedTime.count();

        // controls 
        // handle location
        if (GetAsyncKeyState((unsigned short)'A') & 0x8000)
            fPlayerA -= (1.2f) * fElapsedTime;
        if (GetAsyncKeyState((unsigned short)'D') & 0x8000)
            fPlayerA += (1.2f) * fElapsedTime;
        if (GetAsyncKeyState((unsigned short)'W') & 0x8000)
        {
            fPlayerX += sinf(fPlayerA) * 5.0f * fElapsedTime;
            fPlayerY += cosf(fPlayerA) * 5.0f * fElapsedTime;

            // collision detection (if player hits (coordinate in matrix = '#') a wall undo)
            if (map[(int)fPlayerY * nMapWidth + (int)fPlayerX] == '#')
            {
                fPlayerX -= sinf(fPlayerA) * 5.0f * fElapsedTime;
                fPlayerY -= cosf(fPlayerA) * 5.0f * fElapsedTime;
            }
        }
        if (GetAsyncKeyState((unsigned short)'S') & 0x8000)
        {
            fPlayerX -= sinf(fPlayerA) * 5.0f * fElapsedTime;
            fPlayerY -= cosf(fPlayerA) * 5.0f * fElapsedTime;
            // collision detection (if player hits (coordinate in matrix = '#') a wall undo)
            if (map[(int)fPlayerY * nMapWidth + (int)fPlayerX] == '#')
            {
                fPlayerX += sinf(fPlayerA) * 5.0f * fElapsedTime;
                fPlayerY += cosf(fPlayerA) * 5.0f * fElapsedTime;
            }
        }

        for (int x = 0; x < nScreenWidth; x++)
        {
            // Calculate angle of the ray
            float fRayAngle = (fPlayerA - fFOV / 2.0f) + ((float)x / (float)nScreenWidth) * fFOV;

            float fDistanceToWall = 0.0f;
            bool bHitWall = false;
            bool bBoundary = false;

            float fEyeX = sinf(fRayAngle); // unit vector for ray in player space
            float fEyeY = cosf(fRayAngle);

            vector<pair<float, float>> p; // distance, dot product (angle between vectors)

            while (!bHitWall && fDistanceToWall < fDepth)
            {
                fDistanceToWall += 0.1f;

                int nTestX = (int)(fPlayerX + fEyeX * fDistanceToWall);
                int nTestY = (int)(fPlayerY + fEyeY * fDistanceToWall);

                // Test if ray is out of bounds
                if (nTestX < 0 || nTestY < 0 || nTestX >= nMapWidth || nTestY >= nMapHeight)
                {
                    bHitWall = true;
                    fDistanceToWall = fDepth; // set distance to maximum depth
                }
                else if (map[nTestY * nMapWidth + nTestX] == '#')
                {
                    bHitWall = true;

                    for (int tx = 0; tx < 2; tx++)
                    {
                        for (int ty = 0; ty < 2; ty++)
                        {
                            float vy = (float)nTestY + ty - fPlayerY;
                            float vx = (float)nTestX + tx - fPlayerX;
                            float d = sqrt(vx * vx + vy * vy); // length of vector
                            float dot = (fEyeX * vx / d) + (fEyeY * vy / d); // dot product
                            p.push_back(make_pair(d, dot));
                        }
                    }

           
                    sort(p.begin(), p.end(), [](const pair<float, float>& left, const pair<float, float>& right) { return left.first < right.first; });
                   
                    float fBound = 0.001f; // we are looking for very small angles
                    if (acos(p.at(0).second) < fBound) bBoundary = true; // arc cos of dot product is an angle between two rays
                    if (acos(p.at(1).second) < fBound) bBoundary = true; // these make 3D corner 
                    if (acos(p.at(2).second) < fBound) bBoundary = true;
                }
            }

            // Calculate distance to ceiling and floor
            int nCeiling = (float)(nScreenHeight / 2.0) - nScreenHeight / ((float)fDistanceToWall);
            int nFloor = nScreenHeight - nCeiling;

            // shading
            short nShade = ' ';
            if (fDistanceToWall <= fDepth / 4.0f) nShade = 0x2588; // ASCII for fully shaded char (very close)
            else if (fDistanceToWall < fDepth / 3.0f) nShade = 0x2593;
            else if (fDistanceToWall < fDepth / 2.0f) nShade = 0x2592;
            else if (fDistanceToWall < fDepth) nShade = 0x2591;
            else nShade = ' '; // too far (not shaded)
            
            // shade boundarys of walls 
            if (bBoundary) nShade = ' ';

            for (int y = 0; y < nScreenHeight; y++)
            {
                if (y < nCeiling)
                    screen[y * nScreenWidth + x] = ' ';
                else if (y > nCeiling && y <= nFloor)
                    screen[y * nScreenWidth + x] = nShade;
                else
                {
                    // shade floor based on distance
                    float b = 1.0f - (((float)y - nScreenHeight / 2.0f) / ((float)nScreenHeight / 2.0f));
                    short floorShade = ' ';
                    if (b < 0.25) floorShade = '#';
                    else if (b < 0.5) floorShade = 'x';
                    else if (b < 0.75) floorShade = '.';
                    else if (b < 0.9) floorShade = '-';
                    else floorShade = ' ';
                    screen[y * nScreenWidth + x] = floorShade;
                }
            }
        }

        screen[nScreenWidth * nScreenHeight - 1] = '\0'; // set last char of string as null character
        WriteConsoleOutputCharacter(hConsole, screen, nScreenWidth * nScreenHeight, { 0,0 }, &dwBytesWritten);
    }

    return 0;
}
