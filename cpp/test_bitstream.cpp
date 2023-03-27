#include "range_encoder.h"

int main()
{
    BitStreamDynamic bits;
    for(int i=0;i<100;i++){
        int a = rand()%2;
        cout<<a;
        bits.append(a);
    }
    cout<<endl;
    auto iter = bits.iter();
    for(int i=0;i<108;i++){
        cout<<(int)iter.next();
    }
}