/*
--Node description: 
Subject Result Calculator node receives service requests (srv:SubjectResults) to calculate the
final grade for an exam. This result is returned to the requester.

to test: 
ros2 service call /subject_result groep4_26_assign1_interface_pkg/srv/SubjectResults \
  "{student: 'Sara', subject: 'linux', year: 4, exam_results: [6.0, 8.0]}"
*/ 

/*
--Software changes:
one line per change 
(1) created 31.3.2025: developer-Tilmann Koster reviewer(s)-Niek Ottens 
(2) changed 13.10.2026: changed the template to calculate final grade: developer-Sara Sotomayor Amador
*/


//--general includes
#include <string>
#include <vector>
#include <numeric>
#include "rclcpp/rclcpp.hpp"

//--custom includes
#include "groep4_26_assign1_interface_pkg/srv/subject_results.hpp"

//--using
using SubjectResults = groep4_26_assign1_interface_pkg::srv::SubjectResults;
using namespace std::placeholders;

class SubjectResultCalculator : public rclcpp::Node
{
public:
  SubjectResultCalculator() : Node("subject_result_calculator_node")
  {
    // gewichten per aantal exams (defaults uit de design)
    this->declare_parameter<std::vector<double>>("weights_2", {0.3, 0.7});
    this->declare_parameter<std::vector<double>>("weights_3", {0.2, 0.3, 0.5});
    this->declare_parameter<std::vector<double>>("weights_4", {0.1, 0.2, 0.3, 0.4});

    this->declare_parameter<std::string>("service_name", "subject_result");
    service_ = this->create_service<SubjectResults>("subject_result",
      std::bind(&SubjectResultCalculator::callback, this, _1, _2));
    RCLCPP_INFO(this->get_logger(), "Subject result calculator started");
  }

  void callback(const SubjectResults::Request::SharedPtr request,
                const SubjectResults::Response::SharedPtr response)
  {
    const auto & grades = request->exam_results;
    const size_t n = grades.size();

    if (n == 0) {
      RCLCPP_WARN(this->get_logger(), "No exam results for %s / %s", request->student.c_str(), request->subject.c_str());
      response->result = 0.0;
      return;
    }

    if (n == 1) {
      response->result = grades[0];
    } else if (n <= 4) {
      std::vector<double> weight = this->get_parameter("weights_" + std::to_string(n)).as_double_array();

      double sum = 0.0;
      for (size_t i = 0; i < n; i++){
		sum += grades[i] * weight[i];
	  } 
      response->result = sum;
	  
    } else {
      // 5 of meer: gelijk tellen
      response->result = accumulate(grades.begin(), grades.end(), 0.0) / n;
    }

	response->result = round(response->result * 10.0) / 10.0;
  }

private:
  rclcpp::Service<SubjectResults>::SharedPtr service_;
};


int main(int argc,char *argv[])
{
rclcpp::init(argc,argv)	;
auto node = std::make_shared<SubjectResultCalculator>();
rclcpp::spin(node);
rclcpp::shutdown();

return 0;

}