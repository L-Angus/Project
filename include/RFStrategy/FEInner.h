#ifndef __FEINNER_H__
#define __FEINNER_H__

#include <bitset>
#include <set>

class FEInner {
public:
  FEInner() {}
  ~FEInner() {}

  void SetPort(const std::set<unsigned int> &port) {
    for (auto p : port) {
      mFe.set(p);
    }
  }

  std::bitset<256> GetPort() const { return mFe; }

private:
  std::bitset<256> mFe;
};

#endif // __FEINNER_H__