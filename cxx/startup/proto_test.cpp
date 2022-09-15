#include "common.pb.h"
#include <iostream>

int main() {
    ProtoCommon::Message msg;
    msg.add_type(ProtoCommon::INSERT_ORDER);
    msg.mutable_insert_order()->set_account_name("conor");
    msg.mutable_insert_order()->mutable_instrument()->set_id(0);
    msg.mutable_insert_order()->mutable_instrument()->set_precision(2);
    msg.mutable_insert_order()->set_request_id(0);
    msg.mutable_insert_order()->set_volume(100);
    msg.mutable_insert_order()->set_price(100);
    msg.mutable_insert_order()->set_side(ProtoCommon::BUY);

    std::cout << "msg: " << msg.SerializeAsString() << std::endl;

    return 0;
}