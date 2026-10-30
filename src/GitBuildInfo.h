#pragma once

namespace GitBuildInfo
{
    /**
     * @brief Write the current build's Git/version metadata into the gitinfo partition.
     *
     * The partition starts at flash address 0x3FF000 in gitinfo_4mb.csv. The
     * binary record is 44 bytes; its byte layout is documented in GitBuildInfo.cpp.
     * The partition is only erased and rewritten when the record changes.
     *
     * @return true when the record is already current or was successfully written.
     */
    bool UpdateFlashPartition();
}