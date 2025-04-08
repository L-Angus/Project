#ifndef __DACCFG3_HPP__
#define __DACCFG3_HPP__

#include <stdint.h>

struct DACCFG3_L {
  uint32_t QDAC_FSC_ADJ : 10;
  uint32_t AUX_IDAC_DATA : 10;
  uint32_t QDAC_IDAC_DATA : 10;
  uint32_t DAC_SPI_SELECT : 1;
  uint32_t reserved : 1;
};

struct DACCFG3_H {
  uint32_t I_PHASE_ADJ : 10;
  uint32_t Q_PHASE_ADJ : 10;
  uint32_t IDAC_FSC_ADJ : 10;
  uint32_t AUX_IDAC_SIGN : 1;
  uint32_t AUX_QDAC_SIGN : 1;
};

enum class DAC_SPI_SELECT_TYPE { DAC_SPI_SELECT12, DAC_SPI_SELECT34 };
enum class AUX_DAC_SIGN_TYPE { POSITIVE, NEGATIVE };

class DACCFG3 {
public:
  DACCFG3() : full(0) {}
  void Set_QDAC_FSC_ADJ(uint32_t value) { low.QDAC_FSC_ADJ = value & 0x3FF; }
  void Set_AUX_IDAC_DATA(uint32_t value) { low.AUX_IDAC_DATA = value & 0x3FF; }
  void Set_QDAC_IDAC_DATA(uint32_t value) { low.QDAC_IDAC_DATA = value & 0x3FF; }
  void Set_DAC_SPI_SELECT(DAC_SPI_SELECT_TYPE value) { low.DAC_SPI_SELECT = (uint32_t)value; }

  void Set_I_PHASE_ADJ(uint32_t value) { high.I_PHASE_ADJ = value & 0x3FF; }
  void Set_Q_PHASE_ADJ(uint32_t value) { high.Q_PHASE_ADJ = value & 0x3FF; }
  void Set_IDAC_FSC_ADJ(uint32_t value) { high.IDAC_FSC_ADJ = value & 0x3FF; }
  void Set_AUX_IDAC_SIGN(AUX_DAC_SIGN_TYPE value) { high.AUX_IDAC_SIGN = (uint32_t)value; }
  void Set_AUX_QDAC_SIGN(AUX_DAC_SIGN_TYPE value) { high.AUX_QDAC_SIGN = (uint32_t)value; }

  uint64_t Get() { return full; }

private:
  bool IsDacSpiSelect(DAC_SPI_SELECT_TYPE dac_spi_select) {
    return (dac_spi_select == DAC_SPI_SELECT_TYPE::DAC_SPI_SELECT12 ||
            dac_spi_select == DAC_SPI_SELECT_TYPE::DAC_SPI_SELECT34);
  }

  bool IsAuxIdacSign(AUX_DAC_SIGN_TYPE aux_idac_sign) {
    return (aux_idac_sign == AUX_DAC_SIGN_TYPE::POSITIVE ||
            aux_idac_sign == AUX_DAC_SIGN_TYPE::NEGATIVE);
  }

private:
  union {
    struct {
      DACCFG3_L low;
      DACCFG3_H high;
    };
    uint64_t full;
  };
};
#endif