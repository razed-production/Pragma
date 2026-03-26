#include "Pragma/Core/Application.h"

#include <exception>
#include <iostream>

int main()
{
    try
    {
        Pragma::Core::Application app;
        app.Run();
        return 0;
    }
    catch (const std::exception& ex)
    {
        std::cerr << "Fatal error: " << ex.what() << '\n';
        return 1;
    }
}
