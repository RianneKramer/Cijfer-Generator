
/*
--Node description: 
for now it publishes students of the msg type student 
in terminal to listen: ros2 topic echo /the_answer

*/ 

/*
--Software changes:
one line per change 
(1) created 31.3.2025: developer-Tilmann Koster reviewer(s)-Niek Ottens 
(2) changed 12.10.2026: changed in order to publish students of the type student, developer- Sara Sotomayor Amador
...
*/

//-- tester: Sander Gieling

//--general includes 
#include <cstdlib>
#include "rclcpp/rclcpp.hpp"
#include "groep4_26_assign1_interface_pkg/msg/student.hpp"
#include "groep4_26_assign1_interface_pkg/msg/students.hpp"
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
	 publisher_theanswer_ = this->create_publisher<groep4_26_assign1_interface_pkg::msg::Students>("the_answer",10);
	 timer_template_= this->create_wall_timer(std::chrono::seconds(3),std::bind(&Template_General::publish_students,this));
	 
	 //--customs functions:
		publish_students();
	 //... 
	 
	}

	//-- communication and timer functions 
	//... 

	//--customs functions:
	void publish_students()
	{   
        // Student 1 
        student1_.student = "Sara";
        student1_.subject = {"wiskunde", "robotica", "linux"};
        student1_.nr_exams = 3;
        student1_.year = 4;

        // Student 2
        student2_.student = "Rianne";
        student2_.subject = {"robotica", "linux"};
        student2_.nr_exams = 2;
        student2_.year = 4;

        students_.student = {student1_, student2_};
		publisher_theanswer_ ->publish(students_);
	}

	void random_generator(){
        // for amount of exams produce random exam result
    }

	private:

	//--rclcpp variables:
	rclcpp::Publisher<groep4_26_assign1_interface_pkg::msg::Students>::SharedPtr publisher_theanswer_;
	rclcpp::TimerBase::SharedPtr timer_template_;
		
	//--custom variables:
	groep4_26_assign1_interface_pkg::msg::Student student1_, student2_;
    groep4_26_assign1_interface_pkg::msg::Students students_;

};


int main(int argc,char *argv[])
{

rclcpp::init(argc,argv);
auto node = std::make_shared<Template_General>();
rclcpp::spin(node);
rclcpp::shutdown();

return 0;

}