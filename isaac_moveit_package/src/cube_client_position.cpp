#include "rclcpp/rclcpp.hpp"
#include "isaac_moveit_package/srv/cube_position.hpp"

using GetCubePose = isaac_moveit_package::srv::CubePosition;

class CubeClient : public rclcpp::Node
{
public:
    CubeClient() : Node("cube_client")
    {
        client_ = this->create_client<GetCubePose>("/cube_position");

        while (!client_->wait_for_service(std::chrono::seconds(2))) {
            RCLCPP_WARN(this->get_logger(), "Waiting for service...");
        }

        send_request();
    }

private:
    rclcpp::Client<GetCubePose>::SharedPtr client_;

    void send_request()
    {
        auto request = std::make_shared<GetCubePose::Request>();

        auto future = client_->async_send_request(request,
            std::bind(&CubeClient::response_callback, this, std::placeholders::_1));
    }

    void response_callback(rclcpp::Client<GetCubePose>::SharedFuture future)
    {
        auto response = future.get();

        RCLCPP_INFO(this->get_logger(), "Cube Pose:");
        RCLCPP_INFO(this->get_logger(), "Position: [%.3f, %.3f, %.3f]",
                    response->x, response->y, response->z);
    }
};

int main(int argc, char **argv)
{
    rclcpp::init(argc, argv);
    auto node = std::make_shared<CubeClient>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}