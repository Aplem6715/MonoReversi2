#include "const.h"
#include <iostream>
using namespace std;

int main()
{
    uint64 black, white, mob;
    while (1)
    {
        cin >> black >> white >> mob;

        uint64 cursor = 0x8000000000000000;
        printf("＋ー＋ー＋ー＋ー＋ー＋ー＋ー＋ー＋ー＋\n");
        printf("｜　｜ A｜ B｜ C｜ D｜ E｜ F｜ G｜ H｜\n");
        for (int y = 0; y < 17; y++)
        {
            for (int x = 0; x < 17; x++)
            {
                if (x == 0)
                {
                    if (y % 2 == 0)
                    {
                        printf("＋ー");
                    }
                    else
                    {
                        printf("｜ %d", y / 2 + 1);
                    }
                }

                if (y % 2 == 0)
                {
                    if (x % 2 == 0)
                    {
                        printf("＋");
                    }
                    else
                    {
                        printf("ー");
                    }
                }
                else
                {
                    if (x % 2 == 0)
                    {
                        printf("｜");
                    }
                    else if (cursor & black)
                    {
                        printf("○");
                    }
                    else if (cursor & white)
                    {
                        printf("●");
                    }
                    else if (cursor & mob)
                    {
                        printf("・");
                    }
                    else
                    {
                        printf("　");
                    }
                    if (x % 2 == 1)
                    {
                        cursor >>= 1;
                    }
                }
            }
            printf("\n");
        }
    }

    return 0;
}