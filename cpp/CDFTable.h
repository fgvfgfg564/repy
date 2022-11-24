#ifndef _H_CDFTABLE_
#define _H_CDFTABLE_
#include <bits/stdc++.h>

using namespace std;

double normalCDF(double value)
{
    return 0.5 * erfc(-value * M_SQRT1_2);
}

/*
 * Contains multiple CDF Tables
 */
class CDFTable
{
public:
    int _precision;
    int _n;

    CDFTable(int n, int precision): _n(n), _precision(precision) {}
    virtual int operator()(int idx, int x) const = 0; // gets [P_idx(X < x) * precision]
    virtual int lookup(int idx, int prob) const = 0;
    int getPrecision() const {return _precision;}
};


/*
    Format of a CDF table
    _cdf[x] = P(X<x) * 2^precision

    i.e.
    _cdf[0] = P(x<0) * 2^precision = 0
    _cdf[1] = P(x<1) * 2^precision
    ...
    _cdf[len-2] = P(x<maxv) * 2^precision
    _cdf[len-1] = P(x<maxv+1) * 2^precision = 2 ^ precision

    the length of _cdf equals to maxv - minv + 2
 */

struct ListCDFTableRaw{
    int _precision, _n;
    int *_cdf, *_maxv, *_minv, _max_cdf_length;
};

class ListCDFTable: public CDFTable
{
public:
    int *_cdf;
    int *_maxv;
    int *_minv;
    int _max_cdf_length;

    ListCDFTable(int n, int precision, int cdf[], int minv[], int maxv[], int max_cdf_length): 
        CDFTable(n, precision), _cdf(cdf), _minv(minv), _maxv(maxv), _max_cdf_length(max_cdf_length) {}
    ListCDFTable(const ListCDFTableRaw &raw): CDFTable(raw._n, raw._precision), 
        _cdf(raw._cdf), _maxv(raw._maxv), _minv(raw._minv), _max_cdf_length(raw._max_cdf_length) {}
    int operator()(int idx, int x) const {
        if(idx < 0 || idx >= _n){
            cerr << "CDF table index out of range: " << idx << endl;
            exit(-1);
        }
        if(x < _minv[idx] || x > _maxv[idx]) {
            cerr << "Index out of range: " << x << " in [" << _minv[idx] << "," << _maxv[idx] << "]" << endl;
            exit(-1);
        }
        int *cdf = _cdf + _max_cdf_length * idx;
        x -= _minv[idx];
        return cdf[x];
    }
    int lookup(int idx, int prob) const {
        if(prob < 0 || prob > (1 << _precision)) {
            cerr << "Invalid prob: " << prob << endl;
            exit(-1);
        }
        int *cdf = _cdf + _max_cdf_length * idx;
        return upper_bound(cdf, cdf + _max_cdf_length, prob) - cdf - 1 + _minv[idx];
    }
};

struct GaussianCDFTableRaw
{
    int _precision, _n;
    double *_mu, *_sigma;
};

class GaussianCDFTable: public CDFTable
{
private:
    double *_mu, *_sigma;
public: 
    GaussianCDFTable(int n, int precision, double mu[], double sigma[]): 
        CDFTable(n, precision), _mu(mu), _sigma(sigma) {}
    GaussianCDFTable(const GaussianCDFTableRaw &raw): CDFTable(raw._n, raw._precision), _mu(raw._mu), _sigma(raw._sigma) {}

    int operator()(int idx, int x) const {
        if(idx < 0 || idx >= _n){
            cerr << "CDF table index out of range: " << idx << endl;
            exit(-1);
        }
        int actual_min = floor(_mu[idx] - 6 * _sigma[idx]);
        int actual_max = ceil(_mu[idx] + 6 * _sigma[idx]);
        if (x < actual_min || x > actual_max) {
            cerr << "Gaussian value out of range: " << x << endl;
            exit(-1);
        }
        int multiplier = (1 << _precision) - (actual_max - actual_min + 1);
        double x_new = (x - 0.5 - _mu[idx]) / _sigma[idx];
        double cdf = normalCDF(x_new) * multiplier;
        return round(cdf) + x - actual_min;
    }

    int lookup(int idx, int prob) const {
        if(prob < 0 || prob > (1 << _precision)) {
            cerr << "Invalid prob: " << prob << endl;
            exit(-1);
        }

        int l = floor(_mu[idx] - 6*_sigma[idx]);
        int r = ceil(_mu[idx] + 6*_sigma[idx]);
        while(l < r - 1) {
            int mid = (l+r) >> 1;
            if((*this)(idx, mid) > prob) r = mid;
            else l = mid;
        }
        return l;
    }
};
#endif