#pragma once

#include <memory>
#include <vector>

struct MucCallback {
    typedef std::shared_ptr<MucCallback> Ptr;
    virtual void processMuc(const std::vector<std::vector<int> >& muc) = 0;
    virtual ~MucCallback() {}
};
