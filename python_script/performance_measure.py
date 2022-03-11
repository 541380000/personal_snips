from line_profiler import LineProfiler

def measure_ntimes(times=1):
    def measure_time(func):
        measure_time.times = times
        measure_time.total_times = times
        def run_and_measure(*args, **kwargs):
            if measure_time.times > 0:
                lprofiler = LineProfiler()
                lp_func = lprofiler(func)
                res = lp_func(*args, **kwargs)
                print(f'>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Start of Measuring {func.__name__} '
                      f'[Round {measure_time.total_times - measure_time.times + 1} out of {measure_time.total_times}] '
                      f'>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>')
                lprofiler.print_stats()
                print(f'<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<  End of Measuring {func.__name__} '
                      f'[Round {measure_time.total_times - measure_time.times + 1} out of {measure_time.total_times}]  '
                      f'<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<')
                measure_time.times -= 1
            else:
                return func(*args, **kwargs)
        return run_and_measure
    return measure_time


if __name__ == '__main__':
    # This is a simple demo
    @measure_ntimes(1)
    def func():
        for i in range(10024):
            pass
        print("function done")
    func()
