#ifndef ITABLE_H
#define ITABLE_H
#include <memory>
namespace sqlite
{

    struct ITable
    {
        virtual ~ITable() {}

        virtual std::string Headers(unsigned int col) const = 0;
        virtual std::string Data(unsigned int row, unsigned int col) const = 0;
        virtual size_t Columns() const = 0;
        virtual size_t Rows() const = 0;
    };

    typedef std::shared_ptr<ITable> ITablePtr;

}

#endif // ITABLE_H
