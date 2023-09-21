#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#define MODULE_NAME application_redirect

#include <core/core.h>

using namespace WPEFramework;

namespace WPEFramework {
    namespace Test {
        class RedirectedStream {
        public:
            RedirectedStream(RedirectedStream&&) = delete;
            RedirectedStream(const RedirectedStream&) = delete;
            RedirectedStream& operator=(RedirectedStream&&) = delete;
            RedirectedStream& operator=(const RedirectedStream&) = delete;

            RedirectedStream() = default;
            ~RedirectedStream() = default;

        public:
            void Output(const uint16_t length, const TCHAR buffer[]) {
                string text(buffer, length);
                fprintf(stdout, "Redirected: \"%s\"\n", text.c_str());
            }
        };
    }
}


void help() {
    printf ("W -> Write some text to stderr\n");
    printf ("O -> Open the redirect from stderr\n");
    printf ("C -> Close the redirect from stderr\n");
    printf ("Q -> Quit\n>");
}

int main(int argc, char** argv)
{
    {
        int element;
        uint32_t counter = 0;
        Core::TextStreamRedirectType<Test::RedirectedStream> redirector(fileno(stderr));

        help();
        do {
            element = toupper(getchar());

            switch (element) {
                case 'W': {
                    counter++;
                    fprintf(stdout, "Sending the number [%d] to a potentially redirected stream...\n", counter);
                    fprintf(stderr, "Sending: [%d] ...\n", counter);
                    break;
                }
                case 'O': {
                    fprintf(stdout, "Opening the redirection.\n");
                    redirector.Open();
                    break;
                }
                case 'C': {
                    fprintf(stdout, "Opening the redirection.\n");
                    redirector.Close();
                    break;
                }
                case 'Q': break;
                default: {
                }
            }
        } while (element != 'Q');
    }

    Core::Singleton::Dispose();

    return (0);
}
