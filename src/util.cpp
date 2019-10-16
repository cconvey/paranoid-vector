#include "util.h"

#include <cstring>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <unistd.h>

using namespace std;

std::ostream& operator<<(std::ostream& os, const HexPtr ap) {
    // FIXME: This should'nt assume it knows what the prior stream state was.
    os << std::hex << std::showbase << ap.p << std::dec;
    return os;
}

long get_vm_max_map_count()
{
    ifstream in("/proc/sys/vm/max_map_count");
    in.exceptions(istream::failbit | istream::badbit);

    long x;
    in >> x;
    return x;
}

size_t get_page_size()
{
    long val = sysconf(_SC_PAGESIZE);
    if (val == -1) {
        const string e = std::strerror(errno);
        ostringstream os;
        os << "Unable to determine page size: " << e;
        throw std::runtime_error(os.str());
    }

    return checked_cast<size_t>(val);
}
