#include <bits/stdc++.h>
#include "BPTree.hpp"
#define rep(i,a,b) for(int i=a;i<=b;i++)
using namespace std;

const int N = 50000;

class cmp{
public:
	bool operator()(int a, int b){return a < b;}
};

map<int, int> stdmp;
BPTree<int, int, cmp> tmp("jieti");
int a[N + 100];
//FILE *ttt;

int main() {
	srand(time(0));
	freopen("test.in", "w", stdout);
        //freopen("log.out", "w", ttt);
	rep(i, 1, N) {
                int pos = (abs(rand()) % N) / 5  + 1;
		if (a[pos]) {
                        if (abs(rand()) % 2 == 0){
				printf("remove %d\n", pos);
				a[pos] = 0;
				tmp.removeData(pos);
			}
			else {
				printf("find %d\n", pos);
				if (a[pos] != *(tmp.findU(pos))) {
					printf("error\n");
					printf("%d %d\n", a[pos], *(tmp.findU(pos)));
                                        //fprintf(ttt, "WA\n");
					break;
				}
			}
		}
		else {
                        int t = rand() + 1;
                        printf("ins %d %d\n", pos, t);
			a[pos] = t;
			tmp.insertData(pos, t);
		}
	}
        //fprintf(ttt, "AC\n");
	return 0;
}
