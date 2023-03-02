#include "range_encoder.h"
#include "CDFTable.h"

using namespace std;

#define N 100000
#define C 1024
#define P 16

int a[N];

bool test(int *a, int dim1, int dim2, CDFTable *table) {
    int n = dim1 * dim2;
    cout << "Generated!" << endl;
    BitStreamDynamic bits = encode_single_channel(a, dim1, dim2, *table);
    const int* a_hat = decode_single_channel(bits, dim1, dim2, *table);
    int cmp = memcmp(a, a_hat, n*sizeof(int));
    if(cmp != 0) {
        for(int i=0;i<n;i++) cout<<a[i]<<' ';cout<<endl;
        for(int i=0;i<n;i++) cout<<a_hat[i]<<' ';cout<<endl;
    }
    return cmp == 0;
}

void testList(int dim1, int dim2, int c, int p) {
    int *cdf = new int[dim1 * c];
    int *minv = new int[dim1];
    int *maxv = new int[dim1];

    for(;;){
        for(int idx = 0;idx<dim1;idx++){
            cdf[idx*c] = 0;
            cdf[idx*c+c-1] = 1 << p;
            do{
                for(int i=1;i<=c-2;i++) {
                    cdf[idx*c+i] = rand() % (cdf[idx*c+c-1] - 1);
                }
                sort(cdf+idx*c, cdf+idx*c+c);
                for(int i=1;i<=c-2;i++) {
                    if(cdf[i+idx*c] <= cdf[i-1+idx*c]) cdf[i+idx*c] = cdf[i-1+idx*c] + 1;
                }
            }while(cdf[c-1+idx*c] <= cdf[c-2+idx*c]);

            minv[idx] = 0*(rand() % c) * (1 - 2 * (rand() % 2));
            maxv[idx] = minv[idx] + c - 2;

            for(int i=0;i<dim2;i++) a[i+idx*dim2]=minv[idx] + rand() % (maxv[idx] - minv[idx] + 1);
        }

        ListCDFTable table(dim1, p, cdf, minv, maxv, c);

        // for(int idx=0;idx<dim1;idx++) {
        //     for(int i=0;i<c;i++) {
        //         cout<<cdf[i] << ' ';
        //     }
        //     cout << endl;
        //     for(int i=0;i<(1<<p);i++) {
        //         cout << table.lookup(idx, i) << ' ';
        //     }
        //     cout<<endl;
        // }
        // cout << minv[0] << ' ' << maxv[0] << endl;

        if(test(a, dim1, dim2, &table)) {
            cout << "Passed\n";
        }
        else {
            cout << "Failed\n";
            return;
        }
    }
}

void testGaussian(int n, double mu_range, double sigma_range, int p) {
    for(;;) {
        double *mu = new double[n];
        double *sigma = new double[n];
        for(int i=0;i<n;i++) {
            mu[i] = 1.0 * (rand() & 0xffff) / 0x10000 * mu_range;
            sigma[i] = 1.0 * (rand() & 0xffff) / 0x10000 * sigma_range + 0.11;
        }
        GaussianCDFTable table(n, p, mu, sigma);
        // cout << sigma[0] << endl;
        // for(int i=-4;i<=4;i++){
        //     cout << i << ": " << table(2, i) << endl;
        // }
        // for(int i=0;i<16;i++){
        //     cout << table.lookup(2, i) << endl;
        // }
        for(int i=0;i<n;i++) {
            int mi = floor(mu[i] - 6 * sigma[i]);
            int mx = ceil(mu[i] + 6 * sigma[i]);
            a[i] = mi + rand() % (mx - mi + 1);
        }
        if(test(a, n, 1, &table)) {
            cout << "Passed\n";
        }
        else {
            cout << "Failed\n";
            return;
        }
    }
}

int main()
{
    // testList(5, 5, 3, 4);
    testGaussian(10000, 0, 0, 16);
}