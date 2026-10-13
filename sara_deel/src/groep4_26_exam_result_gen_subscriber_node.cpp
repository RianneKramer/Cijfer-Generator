/*
--Node description: 
this node subscriber to the student publisher and generates random exam results per student and gives that information to 
the message ExamResult and publishes it

!Vraag om te stellen, hoeveel decimalen mag cijfer op eindigen!!

to check -->
1. run launch file first: ros2 launch sara_deel launch.xml
2. then echo (nothing will show the first time!): ros2 topic echo /exam_result
3. then restart the launch file: ros2 launch sara_deel launch.xml

*/ 

/*
--Software changes:
one line per change 
(1) created 31.3.2025: developer-Tilmann Koster reviewer(s)-Niek Ottens 
(2) changed 13.10.2026 developer-Sara Sotomayor Amador
...
*/

//--general includes 
#include <cstdlib>
#include "rclcpp/rclcpp.hpp"
#include <map>
#include <string>
#include <vector>
#include <random>

//--custom includes 
#include "groep4_26_assign1_interface_pkg/msg/student.hpp"
#include "groep4_26_assign1_interface_pkg/msg/students.hpp" 
#include "groep4_26_assign1_interface_pkg/msg/exam_result.hpp"

//--using 
using namespace std::placeholders;

//--Node class 
class ExamResultSubscriber : public rclcpp::Node
{
  public:
  //-- constuctor: 
  ExamResultSubscriber(): Node("exam_result_generator_node")
  {
     //--communication and timer objects: 
    this->declare_parameter<double>("interval_s", 1.0);
    double interval = this->get_parameter("interval_s").as_double();

    subscriber_theanswer_ = this->create_subscription<groep4_26_assign1_interface_pkg::msg::Students>(
      "the_answer", 10,
      std::bind(&ExamResultSubscriber::subscriber_theanswer_callback, this, _1));

    publisher_result_ = this->create_publisher<groep4_26_assign1_interface_pkg::msg::ExamResult>("exam_result", 10);
    timer_ = this->create_wall_timer( std::chrono::duration<double>(interval), std::bind(&ExamResultSubscriber::generate_tick, this));
  }


  //-- communication and timer functions 

  // function subscribes to student publisher 
  void subscriber_theanswer_callback(const groep4_26_assign1_interface_pkg::msg::Students::SharedPtr msg)
  {
    for (const auto & s : msg->student) {
      // emplace doesn't override if student already exists --> doesn't reset the counter
      students_.emplace(s.student, Entry{s.subject, s.nr_exams, s.nr_exams});
    }
  }

  void generate_tick()
{
  for (auto & [name, e] : students_) {
    if (e.remaining <= 0 || e.subjects.empty()) continue;

    //range of the subjects of a student
    std::uniform_int_distribution<size_t> dist(0, e.subjects.size() - 1);

    //filling message ExamResult
    groep4_26_assign1_interface_pkg::msg::ExamResult r;
    r.header.stamp = this->now();
    r.student = name;
    r.subject = e.subjects[dist(rng_)];
    r.exam_result = grade_(rng_);
    r.exam_sequence_nr = e.total - e.remaining + 1;

    // publishing
    RCLCPP_DEBUG(this->get_logger(), "%s: %s -> %.1f (nr %d)",
            r.student.c_str(), r.subject.c_str(), r.exam_result, r.exam_sequence_nr);
    publisher_result_->publish(r);
    e.remaining--;
  }
}
  

private:
  //--rclcpp variables:
  rclcpp::Publisher<groep4_26_assign1_interface_pkg::msg::ExamResult>::SharedPtr publisher_result_;
  rclcpp::Subscription<groep4_26_assign1_interface_pkg::msg::Students>::SharedPtr subscriber_theanswer_;
  rclcpp::TimerBase::SharedPtr timer_;
  std::mt19937 rng_{std::random_device{}()};
  std::uniform_real_distribution<double> grade_{1.0, 10.0}; //range of the grades possible
  //--custom variables:

  struct Entry {
    std::vector<std::string> subjects;
    int remaining; //remaining subjects to fill with a result
    int total; //total subjects
  };

  //lookup table, key = student name
  std::map<std::string, Entry> students_;

  
    
};

int main(int argc, char * argv[])
{
  
rclcpp::init(argc, argv);
auto node = std::make_shared<ExamResultSubscriber>();
rclcpp::spin(node);
rclcpp::shutdown();

return 0;
}
