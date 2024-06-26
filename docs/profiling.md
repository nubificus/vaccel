## Enable profiling

Profiling is an essential aspect of performance optimization in software
development. By enabling profiling, we can instrument our code to identify and
understand sources of overhead. This information is crucial for optimizing
code, improving performance, and ensuring efficient use of resources. In the
context of the vAccel framework, profiling allows developers to track
performance metrics of vAccel API operations and plugins.

### Profiling Stack

vAccel profiling is built around the concept of profiling regions. A region
represents a specific part of the code where performance data is collected. The
profiling stack includes various functions to manage these regions, from
initialization to data collection and reporting.

### Profiling Components

#### Region Initialization and Destruction

**Initialization**: A profiling region is initialized using
`vaccel_prof_region_init()`. This sets up the region with a specified name.

**Destruction**: When a region is no longer needed,
`vaccel_prof_region_destroy()` is used to clean up any resources associated
with it.

#### Start and Stop Profiling

**Start Profiling**: Use `vaccel_prof_region_start()` to begin collecting
performance data for a region.

**Stop Profiling**: To stop data collection, `vaccel_prof_region_stop()` is
used.

#### Data Collection and Reporting

**Sample Collection**: The profiling system collects samples of performance
data during the execution of a region.

**Reporting**: `vaccel_prof_region_print()` is used to output the collected
data for analysis.

#### Batch Operations

**Start by Name**: `vaccel_prof_regions_start_by_name()` allows starting
multiple regions by their names.

**Stop by Name**: `vaccel_prof_regions_stop_by_name()` stops multiple regions
similarly.

**Clear Regions**: `vaccel_prof_regions_clear()` clears collected data from
regions.

**Print All**: `vaccel_prof_regions_print_all()` prints profiling data for all
regions.

#### Enabling Profiling

Profiling in vAccel is enabled by setting an environment variable. To activate
profiling, set the environment variable `VACCEL_PROF_ENABLED` to `enabled`.

```bash
export VACCEL_PROF_ENABLED=enabled
```

### Adding Profiling to Your vAccel API Operation or Plugin

To add profiling to your vaccel API operation or plugin, follow these steps:

#### Include Profiling Headers

Ensure that you include the necessary headers in your code:

```c
#include <vaccel_prof.h>
```

#### Define and Initialize a Profiling Region

Define a `vaccel_prof_region` structure and initialize it:

```c
struct vaccel_prof_region my_region = VACCEL_PROF_REGION_INIT("my_operation");
vaccel_prof_region_init(&my_region, "my_operation");
```

#### Start Profiling

Before the code section you want to profile, start profiling the region:

```c
vaccel_prof_region_start(&my_region);
```

#### Stop Profiling

After the code section, stop profiling the region:

```c
vaccel_prof_region_stop(&my_region);
```

#### Print Profiling Results

To output the profiling results, use:

```c
vaccel_prof_region_print(&my_region);
```

#### Destroy the Profiling Region

Finally, clean up the profiling region:

```c
vaccel_prof_region_destroy(&my_region);
```

### Example Code
```c
#include <vaccel_prof.h>

void my_vaccel_operation() {
    struct vaccel_prof_region my_region = VACCEL_PROF_REGION_INIT("my_operation");
    vaccel_prof_region_init(&my_region, "my_operation");

    vaccel_prof_region_start(&my_region);
    // Your code here
    vaccel_prof_region_stop(&my_region);

    vaccel_prof_region_print(&my_region);
    vaccel_prof_region_destroy(&my_region);
}
```

By following these steps, you can effectively instrument your code with
profiling regions, allowing you to gather valuable performance data and
optimize your vAccel operations and plugins.

Below you can find example output from a torch speech classification tool:

```
$ time ./classifier ./cnn_trace.pt ../bert_cased_vocab.txt 0
2024.06.24-17:49:46.45 - <info> vAccel v0.6.0
2024.06.24-17:49:46.45 - <info> Registered plugin torch 0.1
== [Vocab Loaded] ==
vaccel_single_model_set_path(): Time Taken: 0 milliseconds
vaccel_single_model_register(): Time Taken: 0 milliseconds
Created new model 1
Initialized vAccel session 1
vaccel_preprocess(): Time Taken: 4 milliseconds
2024.06.24-17:49:46.98 - <info> [prof] torch_input: total_time: 127964 nsec nr_entries: 1
2024.06.24-17:49:46.98 - <info> [prof] torch_run: total_time: 180927987 nsec nr_entries: 1
2024.06.24-17:49:46.98 - <info> [prof] torch_out: total_time: 31391 nsec nr_entries: 1
==========Show Result(vAccel)==========
Prediction: offensive-language
Length: 27
Duration: 462
vaccel_preprocess(): Time Taken: 8 milliseconds
2024.06.24-17:49:47.43 - <info> [prof] torch_input: total_time: 162719 nsec nr_entries: 2
2024.06.24-17:49:47.43 - <info> [prof] torch_run: total_time: 321256735 nsec nr_entries: 2
2024.06.24-17:49:47.43 - <info> [prof] torch_out: total_time: 37025 nsec nr_entries: 2
==========Show Result(vAccel)==========
Prediction: offensive-language
Length: 18
Duration: 438
vaccel_preprocess(): Time Taken: 7 milliseconds
2024.06.24-17:49:47.87 - <info> [prof] torch_input: total_time: 198476 nsec nr_entries: 3
2024.06.24-17:49:47.87 - <info> [prof] torch_run: total_time: 457327127 nsec nr_entries: 3
2024.06.24-17:49:47.87 - <info> [prof] torch_out: total_time: 42321 nsec nr_entries: 3
==========Show Result(vAccel)==========
Prediction: offensive-language
Length: 23
Duration: 436
vaccel_preprocess(): Time Taken: 7 milliseconds
2024.06.24-17:49:48.30 - <info> [prof] torch_input: total_time: 233884 nsec nr_entries: 4
2024.06.24-17:49:48.30 - <info> [prof] torch_run: total_time: 586921978 nsec nr_entries: 4
2024.06.24-17:49:48.30 - <info> [prof] torch_out: total_time: 47551 nsec nr_entries: 4
==========Show Result(vAccel)==========
Prediction: neither
Length: 11
Duration: 413
vaccel_preprocess(): Time Taken: 7 milliseconds
2024.06.24-17:49:48.64 - <info> [prof] torch_input: total_time: 269105 nsec nr_entries: 5
2024.06.24-17:49:48.64 - <info> [prof] torch_run: total_time: 721709173 nsec nr_entries: 5
2024.06.24-17:49:48.64 - <info> [prof] torch_out: total_time: 53077 nsec nr_entries: 5
==========Show Result(vAccel)==========
Prediction: offensive-language
Length: 28
Duration: 333
vaccel_preprocess(): Time Taken: 8 milliseconds
2024.06.24-17:49:48.98 - <info> [prof] torch_input: total_time: 304956 nsec nr_entries: 6
2024.06.24-17:49:48.98 - <info> [prof] torch_run: total_time: 855291441 nsec nr_entries: 6
2024.06.24-17:49:48.98 - <info> [prof] torch_out: total_time: 58215 nsec nr_entries: 6
==========Show Result(vAccel)==========
Prediction: offensive-language
Length: 21
Duration: 329
vaccel_preprocess(): Time Taken: 7 milliseconds
2024.06.24-17:49:49.29 - <info> [prof] torch_input: total_time: 340191 nsec nr_entries: 7
2024.06.24-17:49:49.29 - <info> [prof] torch_run: total_time: 988087659 nsec nr_entries: 7
2024.06.24-17:49:49.29 - <info> [prof] torch_out: total_time: 63358 nsec nr_entries: 7
==========Show Result(vAccel)==========
Prediction: offensive-language
Length: 22
Duration: 310
vaccel_preprocess(): Time Taken: 8 milliseconds
2024.06.24-17:49:49.63 - <info> [prof] torch_input: total_time: 375959 nsec nr_entries: 8
2024.06.24-17:49:49.63 - <info> [prof] torch_run: total_time: 1119080025 nsec nr_entries: 8
2024.06.24-17:49:49.63 - <info> [prof] torch_out: total_time: 68831 nsec nr_entries: 8
==========Show Result(vAccel)==========
Prediction: offensive-language
Length: 15
Duration: 327
vaccel_preprocess(): Time Taken: 7 milliseconds
2024.06.24-17:49:49.94 - <info> [prof] torch_input: total_time: 412773 nsec nr_entries: 9
2024.06.24-17:49:49.94 - <info> [prof] torch_run: total_time: 1249991356 nsec nr_entries: 9
2024.06.24-17:49:49.94 - <info> [prof] torch_out: total_time: 74032 nsec nr_entries: 9
==========Show Result(vAccel)==========
Prediction: offensive-language
Length: 15
Duration: 303
vaccel_preprocess(): Time Taken: 8 milliseconds
2024.06.24-17:49:50.25 - <info> [prof] torch_input: total_time: 446881 nsec nr_entries: 10
2024.06.24-17:49:50.25 - <info> [prof] torch_run: total_time: 1380077421 nsec nr_entries: 10
2024.06.24-17:49:50.25 - <info> [prof] torch_out: total_time: 79266 nsec nr_entries: 10
==========Show Result(vAccel)==========
Prediction: neither
Length: 8
Duration: 301
Total Time Taken: 3804 milliseconds
Total nr of Inference invocations: 10
Length: 8
Average: 231

real	0m4.228s
user	0m4.578s
sys	0m1.235s
```

and the respective code:

```c
struct vaccel_prof_region torch_input_stats =
        VACCEL_PROF_REGION_INIT("torch_input");
struct vaccel_prof_region torch_run_stats =
        VACCEL_PROF_REGION_INIT("torch_run");
struct vaccel_prof_region torch_out_stats =
        VACCEL_PROF_REGION_INIT("torch_out");

[..]
    vaccel_prof_region_start(&torch_input_stats);

    std::vector<torch::jit::IValue> input;
    torch::jit::IValue output_ori;
    torch::Tensor input_ids;
    torch::Tensor masks;

    size_t input_outer_size = in_tensors[0]->size;
    size_t masks_outer_size = in_tensors[1]->size;

[..]

    input_ids = input_ids.to(device);
    masks = masks.to(device);

    vaccel_prof_region_stop(&torch_input_stats);

    input.push_back(input_ids);
    input.push_back(masks);

    vaccel_prof_region_start(&torch_run_stats);
    output_ori = model_trace.forward(input);
    vaccel_prof_region_stop(&torch_run_stats);

[..]

    vaccel_prof_region_start(&torch_out_stats);

    ret = tensors_to_vaccel(output, out_tensors, nr_outputs);
    float* data = static_cast<float*>((*out_tensors)->data);
    int64_t num_elements = output.numel();

    vaccel_prof_region_stop(&torch_out_stats);	
    
    vaccel_prof_region_print(&torch_input_stats);
    vaccel_prof_region_print(&torch_model_stats);
    vaccel_prof_region_print(&torch_run_stats);
    vaccel_prof_region_print(&torch_out_stats);
```


