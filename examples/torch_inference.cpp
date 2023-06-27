#include <session.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <vaccel.h>
extern "C" {
#include "../src/utils.h"
}
#include <iostream>
#include <vector>
#include <random>
#include <cstring>

std::vector < float >
random_input_generator (int min_value = 1,
			int max_value = 100,
			size_t vector_size = 150528, bool is_print = true)
{
  // Create a random number generator engine
  std::random_device rd;
  std::mt19937 rng (rd ());

  std::uniform_int_distribution < int >distribution (min_value, max_value);

  // Create a vector and fill it with random numbers
  std::vector < float >res_data (vector_size);
  for (size_t i = 0; i < vector_size; ++i)
    {
      res_data[i] = distribution (rng);
    }

  if (is_print)
    {
      // Print the vector contents
      std::cout << "The first Random numbers:";
    for (int value:res_data)
	{
	  std::cout << " " << value;
	  break;
	}
      std::cout << std::endl;
    }
  return res_data;
}

int
main (int argc, char **argv)
{
  struct vaccel_session sess;
  struct vaccel_torch_saved_model model;
  struct vaccel_torch_buffer run_options;
  float *offsets;
  int ret;
  char *model_path;
  std::vector < float >res_data = random_input_generator ();
  res_data.resize (3 * 224 * 224);
  int64_t dims[] = { 1, 224, 224, 3 };
  struct vaccel_torch_tensor *in;
  struct vaccel_torch_tensor *out;

  if (argc != 3)
    {
      fprintf (stderr, "Usage: %s image filename, model path\n", argv[0]);
      exit (1);
    }

  model_path = strdup (argv[2]);

  ret = vaccel_torch_saved_model_set_path (&model, model_path);
  if (ret)
    {
      fprintf (stderr, "Could not set model path to Torch model %d\n", ret);
      exit (1);
    }
  ret = vaccel_torch_saved_model_register (&model);
  if (ret)
    {
      fprintf (stderr, "Could not register Torch model resource\n");
      exit (1);
    }
  printf ("Created new model %lld\n", vaccel_torch_saved_model_id (&model));

  /* Read the image file */
  ret = read_file (argv[1], (void **)&run_options.data, &run_options.size);
  if (ret)
    {
      fprintf (stderr, "Could not load the image file: %d", ret);
      goto close_session;
    }

  ret = vaccel_sess_init (&sess, 0);
  if (ret)
    {
      fprintf (stderr, "Could not initialize vAccel session\n");
      goto destroy_resource;
    }

  printf ("Initialized vAccel session %u\n", sess.session_id);

  /* Take vaccel_session and vaccel_resource as inputs, 
   * register a resource with a session */
  ret = vaccel_sess_register (&sess, model.resource);
  if (ret)
    {
      fprintf (stderr, "Could not register model with session\n");
      goto close_session;
    }


  in = vaccel_torch_tensor_new (4, dims, VACCEL_TORCH_FLOAT);
  if (!in)
    {
      fprintf (stderr, "Could not allocate memory\n");
      goto close_session;
    }
  in->data = res_data.data ();
  // memcpy(in->data, res_data.data(), res_data.size());
  in->size = res_data.size () * sizeof (float);
  std::cout << "The IN ADDRESS: " << in->data << std::endl;
  std::cout << "size: " << in->size << std::endl;

  /* Output tensor */
  out =
    (struct vaccel_torch_tensor *)
    malloc (sizeof (struct vaccel_torch_tensor) * 1);
  // struct vaccel_torch_tensor *out = vaccel_torch_tensor_new(4, dims, VACCEL_TORCH_FLOAT);
  /* Conducting torch inference */
  ret = vaccel_torch_jitload_forward (&sess,
				      &model, &run_options, &in, 1, &out, 1);

  if (ret)
    {
      fprintf (stderr, "Could not run op: %d\n", ret);
      goto close_session;
    }

  printf ("Success!\n");
  printf ("Result Tensor :\n");
  printf ("Output tensor => type:%u nr_dims:%u\n", out->data_type,
	  out->nr_dims);

  offsets = (float *) out->data;
  //printf ("%d\n", *(int *) out->data);
  printf ("%f\n", *offsets);
  /*
     const char* classes[10] = { "plane", "car", "bird", 
     "cat", "deer", "dog", 
     "frog", "horse", "ship", "truck" };

     printf("Pred: %s\n", classes[int(offsets)])
   */
  ret = vaccel_sess_unregister (&sess, model.resource);
  if (ret)
    {
      fprintf (stderr, "Could not unregister model with session\n");
      // goto close_session;
      return vaccel_sess_free (&sess);
    }

close_session:
  if (vaccel_sess_free (&sess) != VACCEL_OK)
    {
      fprintf (stderr, "Could not clear session\n");
      return 1;
    }
destroy_resource:
  vaccel_torch_saved_model_destroy (&model);
  return ret;
}
