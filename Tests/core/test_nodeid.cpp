#include "../IPTestAdministrator.h"

#include <gtest/gtest.h>
#include <core/core.h>

using namespace WPEFramework;

TEST(Core_NodeId, simpleSet)
{
   Core::NodeId nodeId("localhost:80");

   std::cout << "Hostname:           " << nodeId.HostName() << std::endl;
   std::cout << "Type:               " << nodeId.Type() << std::endl;
   std::cout << "PortNumber:         " << nodeId.PortNumber() << std::endl;
   std::cout << "IsValid:            " << nodeId.IsValid() << std::endl;
   std::cout << "Size:               " << nodeId.Size() << std::endl;
   std::cout << "HostAddress:        " << nodeId.HostAddress() << std::endl;
   std::cout << "QualifiedName:      " << nodeId.QualifiedName() << std::endl;
   std::cout << "IsLocalInterface:   " << nodeId.IsLocalInterface() << std::endl;
   std::cout << "IsAnyInterface:     " << nodeId.IsAnyInterface() << std::endl;
   std::cout << "IsMulticast:        " << nodeId.IsMulticast() << std::endl;
   std::cout << "DefaultMask:        " << static_cast<int>(nodeId.DefaultMask()) << std::endl;
}

