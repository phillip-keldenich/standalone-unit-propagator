#ifndef SP_UNSAT_EXCEPTION_H_INCLUDED_
#define SP_UNSAT_EXCEPTION_H_INCLUDED_

#include <stdexcept>
#include <exception>

namespace sprop {

class UNSATException : public std::exception {
    public:
        UNSATException() = default;
    
        const char* what() const noexcept override {
            return "UNSAT";
        }
};

}

#endif
