/*
--Node description: 
...what the node is doing (functionally)... 
When node is created the node prints "Hello you are new to ROS2" on terminal, 
further it does nothing more than being alive
*/ 

/*
--Software changes:
one line per change 
(1) created 31.3.2025: developer-Tilmann Koster reviewer(s)-Niek Ottens 
(2) changed 01.4.2025: xxx functionality added ... : developer-Tilmann Koster reviewer(s)-Niek Ottens 
...
*/


//-- tester: Sander Gieling



//--general includes 
#include <cstdlib>
#include "rclcpp/rclcpp.hpp"
#include <rclcpp/clock.hpp>
#include <rclcpp/time.hpp>

//--custom includes 
//...

//--using 
//...


//--Node class 

class Template_General : public rclcpp::Node
{
	public:
	//-- constuctor: 
	Template_General() : Node("template_general_node")
	{
	 //--communication and timer objects: 
	 //see templates for subcribers, action server ... 
	 //...  
	 
	 //--customs functions:
		custom_example();
	 //... 
	 
	}

	//-- communication and timer functions 
	//... 

	//--customs functions:
	void custom_example()
	{   
		

		/*your custom example code */
		 RCLCPP_INFO(this->get_logger() ,"Hello you are new to ROS2"); //code example
		/*your custom example code */
		my_time_ = this->now();
		RCLCPP_INFO(this->get_logger() ,"Time in seconds: %f", my_time_.seconds()); 


	}

	//--custom variables: 

	private:

	//--rclcpp variables:
	rclcpp :: Time my_time_;
	rclcpp :: Clock my_clock_;
		
	//--custom variables:
	//...

};


int main(int argc,char *argv[])
{
rclcpp::init(argc,argv)	;

auto node = std::make_shared<Template_General>();


rclcpp::spin(node);

rclcpp::shutdown();

return 0;

}