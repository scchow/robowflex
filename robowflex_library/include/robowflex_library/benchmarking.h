#ifndef ROBOWFLEX_BENCHMARKING_
#define ROBOWFLEX_BENCHMARKING_

#include <boost/variant.hpp>
#include <boost/lexical_cast.hpp>

namespace robowflex
{
    ROBOWFLEX_CLASS_FORWARD(BenchmarkOutputter);
    ROBOWFLEX_CLASS_FORWARD(Benchmarker);

    class Benchmarker
    {
    public:
        enum RunMetricBits
        {
            WAYPOINTS = 1 << 0,
            PATH = 1 << 1,
            CORRECT = 1 << 2,
            LENGTH = 1 << 3,
            CLEARANCE = 1 << 4,
            SMOOTHNESS = 1 << 5,
        };

        class Options
        {
        public:
            Options() : runs(100), run_metric_bits(~0)
            {
            }

            Options(const Options &options) : runs(options.runs), run_metric_bits(options.run_metric_bits)
            {
            }

            unsigned int runs;
            uint32_t run_metric_bits;
        };

        class Results
        {
        public:
            class Run
            {
            public:
                class toString : public boost::static_visitor<const std::string>
                {
                public:
                    const std::string operator()(int value) const
                    {
                        return boost::lexical_cast<std::string>(boost::get<int>(value));
                    }

                    const std::string operator()(double value) const
                    {
                        double v = boost::get<double>(value);
                        return boost::lexical_cast<std::string>(
                            (std::isfinite(v)) ? v : std::numeric_limits<double>::max());
                    }

                    const std::string operator()(bool value) const
                    {
                        return boost::lexical_cast<std::string>(boost::get<bool>(value));
                    }
                };

                Run(int num, double time, bool success) : num(num), time(time), success(success)
                {
                }

                int num;
                double time;
                bool success;
                moveit_msgs::RobotTrajectory path;

                std::map<std::string, boost::variant<bool, double, int>> metrics;
            };

            Results(const std::string &name, const SceneConstPtr scene, const PlannerConstPtr planner,
                    const MotionRequestBuilderConstPtr builder, const Options &options)
              : name(name), scene(scene), planner(planner), builder(builder), options(options)
            {
                start = IO::getDate();
            }

            void addRun(int num, double time, planning_interface::MotionPlanResponse &run);
            void computeMetric(planning_interface::MotionPlanResponse &run, Run &metrics);

            const std::string name;
            const SceneConstPtr scene;
            const PlannerConstPtr planner;
            const MotionRequestBuilderConstPtr builder;
            const Options options;

            boost::posix_time::ptime start;
            boost::posix_time::ptime finish;

            std::vector<Run> runs;
        };

        Benchmarker();

        void addBenchmarkingRequest(const std::string &name, const ScenePtr &scene, const PlannerPtr &planner,
                                    const MotionRequestBuilderPtr &request);

        void benchmark(const std::vector<BenchmarkOutputterPtr> &output, const Options &options = Options());

    private:
        std::map<std::string, std::tuple<ScenePtr, PlannerPtr, MotionRequestBuilderPtr>> requests_;
    };

    class BenchmarkOutputter
    {
    public:
        virtual ~BenchmarkOutputter() = default;

        // Write one unit of output (usually a single planner) to the output.
        virtual void dumpResult(const Benchmarker::Results &results) = 0;
    };

    class JSONBenchmarkOutputter : public BenchmarkOutputter
    {
    public:
        JSONBenchmarkOutputter(const std::string &file) : file_(file)
        {
        }

        ~JSONBenchmarkOutputter() override;

        void dumpResult(const Benchmarker::Results &results) override;

    private:
        bool is_init_{false};
        const std::string file_;
        std::ofstream outfile_;
    };

    class TrajectoryBenchmarkOutputter : public BenchmarkOutputter
    {
    public:
        TrajectoryBenchmarkOutputter(const std::string &file) : file_(file), bag_(file_)
        {
        }

        void dumpResult(const Benchmarker::Results &results) override;

    private:
        /**
         * The trajectories found by each request are stored in a topic of
         * the request name in a bag file of the given name.
         */
        bool is_init_{false};
        const std::string file_;
        IO::Bag bag_;
    };

    class OMPLBenchmarkOutputter : public BenchmarkOutputter
    {
    public:
        OMPLBenchmarkOutputter(const std::string &prefix) : prefix_(prefix)
        {
        }

        void dumpResult(const Benchmarker::Results &results) override;

    private:
        const std::string prefix_;
    };
}  // namespace robowflex

#endif
