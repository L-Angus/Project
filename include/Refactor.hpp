#ifndef RFSTRATEGY_HPP
#define RFSTRATEGY_HPP

void Init() {
  [this](const std::string &stim_type) {
    if (stim_type == "MOD") {
      for (const auto &board : Frame::GetInstance().GetBoards()) {
        if (LD_BOARD != board->GetBoardType())
          continue;
        auto ldSlot = board.GetSlot() - 1;
        STREAM_IN inStream;
        STREAM_OUT outStream;
        inStream << TX_MOD_CPLD_OFFSET << TX_MOD_CPLD_DATA;
        at_func::SendRequestToSlot(ldSlot, "CPLD_CTRL", inStream, outStream, 1000);
      }
    }
  }(this->type);
}

#endif