
#include "includes.h"
void WilsonDirac( gsl_vector_complex * output,gsl_vector_complex * input, void * ctxt)
{
  lattice * L = (lattice * ) ctxt;
  gsl_matrix_complex * gamma_matrix[4];
  gsl_matrix_complex * identity_gamma_minus[4];
  gsl_matrix_complex * identity_gamma_plus[4];
  gsl_matrix_complex * temp_link_conj = gsl_matrix_complex_alloc(2,2);
  gsl_matrix_complex * temp_mat1 = gsl_matrix_complex_alloc(8,8);
  gsl_matrix_complex * temp_mat2 = gsl_matrix_complex_alloc(8,8);
  gsl_vector_complex * temp_vec1 = gsl_vector_complex_alloc(8);
  gsl_vector_complex * temp_vec2 = gsl_vector_complex_alloc(8);

  gsl_vector_complex_view output_vector_view;
  gsl_vector_complex_view input_vector_view1;
  gsl_vector_complex_view input_vector_view2;


  long unsigned site_index, vector_site_index, temp_site_index, temp_vector_site_index;
  unsigned int dir;
  //Begin testing input vectors and matrices
  if(!gamma_matrix[0])
    compute_gamma_matrices();
  if(!identity_gamma_plus[0] || !identity_gamma_minus[0])
    compute_gamma_identities();
  if(!output)
    output = gsl_vector_complex_alloc(8*N4);

  if(!input)
  {
    printf("Input vector has not been allocated.\n");
    exit(0);
  }

  if(input->size != output->size )
  {
    printf("Input vector and output vector have mismatched lengths\n");
    exit(0);
  }
  //End vec/matrix testing
  //If changing the value of m in the WD operator, this is where you'll do it
  gsl_vector_complex_memcpy(output,input)
  gsl_vector_complex_set_all(output,gsl_complex_rect(5.0,0.0));
  FORALLSITES(site_index)
  {
    vector_site_index = 8 * site_index;
    output_vector_view = gsl_vector_complex_subvector(output, vector_site_index, 8 );
    gsl_vector_complex_set_zero(temp_vec1);
    FORALLDIR(dir)
    {
      temp_site_index = hop_index(site_index,1, dir, BACKWARD);
      input_vector_view1 = gsl_vector_complex_subvector(input, temp_site_index, 8 );
      outer_product(temp_mat1,identity_gamma_minus[dir],L->R[site_index]->link[dir]);

      temp_site_index = hop_index(site_index,1, dir, FORWARD);
      input_vector_view2 = gsl_vector_complex_subvector(input, temp_site_index, 8 );
      //Add something to do the Hermitian conjugate to this link
      hermitian_conj(temp_link_conj,L->R[site_index]->link[dir])
      outer_product(temp_mat2,identity_gamma_plus[dir],temp_link_conj);

      gsl_blas_zgemv(CblasNoTrans, gsl_complex_rect(-0.5,0.0), temp_mat1, &input_vector_view1.vector, gsl_complex_rect(1.0,0.0), &output_vector_view.vector);
      gsl_blas_zgemv(CblasNoTrans, gsl_complex_rect(-0.5,0.0), temp_mat2, &input_vector_view2.vector, gsl_complex_rect(1.0,0.0), &output_vector_view.vector);
    }

      gsl_matrix_complex_free(temp_link_conj);
      gsl_matrix_complex_free(temp_mat1);
      gsl_matrix_complex_free(temp_mat2);
      gsl_matrix_complex_free(temp_vec1);
      gsl_matrix_complex_free(temp_vec2);
  }
}

gsl_vector_complex * randomComplexVector(long unsigned length)
{
  gsl_vector_complex * temp = gsl_vector_complex_calloc(length);

   gsl_rng * r;
   const gsl_rng_type * T;
   gsl_rng_env_setup();
   T = gsl_rng_default;
   r = gsl_rng_alloc(T);
   gsl_rng_set(r,time_seed());

   unsigned long filling,i;
   filling =  gsl_rng_uniform_int(r, length);

   for ( i=0; i< filling;i++)
   {
     gsl_vector_complex_set(temp, gsl_rng_uniform_int(r, length), gsl_complex_rect( gsl_ran_gaussian( r, 1.0) ,gsl_ran_gaussian( r, 1.0) ) );
   }
   normalize(temp,temp);
   return temp;
}

void hermitian_conj(gsl_matrix_complex *out, gsl_matrix_complex *in)
{
  gsl_matrix_complex *temp = gsl_matrix_complex_alloc(in->size1,in->size2);
  int i, j;
  for(i=0;i<in->size1;i++)
  {
    for (j = 0; j < in->size2; j++)
    {
      gsl_matrix_complex_set(temp,i,j,gsl_complex_conjugate(gsl_matrix_complex_get(in,j,i))   );
    }
  }
  gsl_matrix_complex_memcpy(out,temp);
  gsl_matrix_complex_free(temp);
}


//w = u cross v
void outer_product(gsl_matrix_complex * w,gsl_matrix_complex * u,gsl_matrix_complex * v)
{
  int i,j;
  int xshift, yshift;
  gsl_matrix_complex_view temp_matrix_view;
  gsl_matrix_complex temp = gsl_matrix_complex_alloc(v->size1,v->size2);
  if(!w)
  {
    printf("Return matrix has not been properly allocated\n");
    exit(0);
  }
  if( !(u->size1 * v->size1 == w->size1 && u->size2 * v->size2 == w->size2))
  {
    printf("Return matrix does not have proper dimensions to compute outer product\n");
    exit(0);
  }
  xshift = v->size1;
  yshift = v->size2;

  for(i=0;i<u->size1;i++)
  {
    for(j=0;j<u->size2;j++)
    {
      temp_matrix_view = gsl_matrix_complex_submatrix(w,i*xshift,j*yshift,xshift,yshift);
      gsl_matrix_complex_memcpy(temp, v);
      gsl_matrix_complex_scale(temp, gsl_matrix_complex_get(u,i,j));
      gsl_matrix_complex_memcpy(&temp_matrix_view->matrix, temp);
    }
  }
}

void compute_gamma_matrices()
{
  int i;
  gsl_matrix_complex * gamma_matrix[4];
  FORALLDIR(i)
  {
    gamma_matrix[i] = gsl_matrix_complex_calloc(4,4);
  }
  //gamma_0 really gamma_4
  gsl_matrix_complex_set(gamma_matrix[0],2,0,gsl_complex_rect(1.0,0.0));
  gsl_matrix_complex_set(gamma_matrix[0],3,2,gsl_complex_rect(1.0,0.0));
  gsl_matrix_complex_set(gamma_matrix[0],0,0,gsl_complex_rect(1.0,0.0));
  gsl_matrix_complex_set(gamma_matrix[0],2,3,gsl_complex_rect(1.0,0.0));
  //gamma_1
  gsl_matrix_complex_set(gamma_matrix[1],3,0,gsl_complex_rect(0.0,-1.0));
  gsl_matrix_complex_set(gamma_matrix[1],2,1,gsl_complex_rect(0.0,-1.0));
  gsl_matrix_complex_set(gamma_matrix[1],0,3,gsl_complex_rect(0.0,1.0));
  gsl_matrix_complex_set(gamma_matrix[1],1,2,gsl_complex_rect(0.0,1.0));
  //gamma_2
  gsl_matrix_complex_set(gamma_matrix[2],3,0,gsl_complex_rect(1.0,0.0));
  gsl_matrix_complex_set(gamma_matrix[2],2,1,gsl_complex_rect(-1.0,0.0));
  gsl_matrix_complex_set(gamma_matrix[2],0,3,gsl_complex_rect(1.0,0.0));
  gsl_matrix_complex_set(gamma_matrix[2],1,2,gsl_complex_rect(-1.0,0.0));
  //gamma_3
  gsl_matrix_complex_set(gamma_matrix[3],2,0,gsl_complex_rect(0.0,-1.0));
  gsl_matrix_complex_set(gamma_matrix[3],3,2,gsl_complex_rect(0.0,-1.0));
  gsl_matrix_complex_set(gamma_matrix[3],0,2,gsl_complex_rect(0.0,1.0));
  gsl_matrix_complex_set(gamma_matrix[3],2,3,gsl_complex_rect(0.0,1.0));
}
void compute_gamma_identities()
{
  int i;
  gsl_matrix_complex * gamma_matrix[4];
  gsl_matrix_complex * identity_gamma_minus[4];
  gsl_matrix_complex * identity_gamma_plus[4];
  gsl_matrix_complex * my_identity = gsl_matrix_complex_calloc(4,4);
  gsl_matrix_complex_set_identity(my_identity);
  FORALLDIR(i)
  {
    identity_gamma_plus[i] = gsl_matrix_complex_calloc(4,4);
    identity_gamma_minus[i] = gsl_matrix_complex_calloc(4,4);
    gsl_matrix_complex_memcpy(identity_gamma_minus,my_identity);
    gsl_matrix_complex_memcpy(identity_gamma_plus,my_identity);
    gsl_matrix_complex_add(identity_gamma_plus,gamma_matrix[i]);
    gsl_matrix_complex_sub(identity_gamma_minus,gamma_matrix[i]);
  }
}

void Lanczos( void (*MV) (gsl_vector_complex *,gsl_vector_complex *, void * ctxt),gsl_vector_complex * q_0 , const int k, void * data    )
{
  //The ctxt argument is a generic pointer so I can re-use Lanczos later for other things
  int j;
  double beta,alpha;
  gsl_matrix_complex * Q = gsl_matrix_complex_alloc(q_0->size,k+1);
  gsl_matrix_complex * S = gsl_matrix_complex_calloc(k,k);
  gsl_vector_complex * r = gsl_vector_complex_alloc(q_0->size);
  gsl_vector_complex * q = gsl_vector_complex_alloc(q_0->size);
  gsl_vector_complex * q_p = gsl_vector_complex_alloc(q_0->size);
  gsl_matrix_complex_view T_view;
  gsl_matrix_complex_view S_view;

  gsl_vector_complex_memcpy(r,q_0);
  gramSchmidtInsertion(r,Q,0);
  beta = GSL_REAL( innerProduct(r,r)  );
  T = constructT(k);
  //2. Begin Loop
  gsl_vector_complex_memcpy(q,r);
  gsl_vector_complex_memcpy(q_p,r);
  for(j=1;j<k;j++)
  {
    //3. Normalize vector q_j = r /beta_{j-1}- This is done automatically in the GSinsertation
    //4. Compute r = Aq_j - q_{j-1} beta_{j-1}
    MV(r,q,data);
    vectorscaleSub(r,q_p,beta);
    //5. alpha_j = q^T_j  r
    alpha = GSL_REAL(innerProduct(q,r));
    //6. r = r - q_j alpha _j
    vectorScaleSub(r,q,alpha);
    //7. Orthogonalize r against Q
    beta = Magnitude(r);
    gramSchmidtInsertion(r,Q,j);
    //8. beta_j = ||r||
    //9. Compute eigenvalues of T_j and Test for convergence
    T_view = gsl_matrix_complex_submatrix(T,0,0,j,j);
    S_view = gsl_matrix_complex_submatrix(S,0,0,j,j);
    appendT(T, alpha, beta,j-1);
    QRalgorithm(&S_view.matrix,eigenvalues, &T_view.matrix,j);
    if(convergenceTest(S,beta,j) < 1.0E-10);
      printf("Eigenvalues converged for j = %d\n",j );
    gsl_vector_complex_memcpy(q_p,q);
    gsl_vector_complex_memcpy(q,r);

  }
  //11. End loop
}


gsl_matrix * constructT(k)
{
  gsl_matrix * temp = gsl_matrix_calloc(k,k);
  return temp;
}

//v <- v - c * u
void vectorScaleSub(gsl_vector_complex * v,gsl_vector_complex * u, gsl_complex c)
{
  gsl_vector_axpby(gsl_complex_rect(1.0,0.0), v, gsl_complex_negative(c), u);
}

// Orthonormalizes r against the previous k-1 orthonormal vectors
// then inserts r into the kth (index i=k-1) column vector of Q
void gramSchmidtInsertion(gsl_vector_complex * r,gsl_matrix_complex * Q, int k)
{
  gsl_vector_complex_view temp_column;
  //The columns of q are the vectors that we need to orthogonalize r against
  //k is the column in Q where the new vector will be inserted
  temp_column = gsl_matrix_complex_column(Q,k);
  gsl_vector_complex_memcpy(&temp_column.vector, r);
  gramSchmidtStep(Q,k);
  gsl_vector_complex_memcpy(r, &temp_column.vector);
}


void appendT(gsl_matrix * T, double alpha, double beta, int j, int k)
{
  if(j<k-1)
  {
    gsl_matrix_set(T,j,j,alpha);
    gsl_matrix_set(T,j+1,j,beta);
    gsl_matrix_set(T,j,j+1,beta);
  }
  else
  {
    gsl_matrix_set(T,j,j,alpha);
  }
}

//j indicates the index location of the most recent addition to T
int QRalgorithm(gsl_matrix * S, gsl_vector * eig, gsl_matrix * T,const int j)
{
  gsl_matrix_view temp_T = gsl_matrix_submatrix(T,0,0,j,j);
  gsl_vector_view diag_view;
  gsl_matrix * R = gsl_matrix_calloc(j,j);
  gsl_matrix * Q = gsl_matrix_alloc(j,j);
  gsl_matrix * Q_total = gsl_matrix_alloc(j,j);
  gsl_matrix * Q_temp = gsl_matrix_alloc(j,j);
  gsl_matrix * A = gsl_matrix_alloc(j,j);
  gsl_matrix * B = gsl_matrix_alloc(j,j);
  gsl_matrix * temp_holding_matrix = gsl_matrix_alloc(j,j);
  int count = 0;

  gsl_matrix_memcpy(A,&temp_T.matrix);
  gsl_matrix_complex_set_identity(Q_total);
  do
  {
    gsl_linalg_QR_decomp_r(A, temp_holding_matrix);
    gsl_linalg_QR_unpack_r(A, temp_holding_matrix, Q,R);
    gsl_blas_dgemm(CblasNoTrans, CblasNoTrans, 1.0d, R, Q, 0.0d, B);
    gsl_blas_dgemm(CblasNoTrans, CblasNoTrans, 1.0d, Q_total, Q, 0.0d, Q_temp);
    gsl_matrix_memcpy(A,B);
    gsl_matrix_memcpy(Q_total,Q_temp);
    count++;
    if(count > 9999)
    {
      printf("QR algorithm didn't converge\n");
      exit(0);
    }
  }while(!QRconvergence(A));

  diag_view = gsl_matrix_diagonal(A);
  gsl_vector_memcpy(eig,&diag_view.vector);
  gsl_matrix_memcpy(S,Q_total);

  gsl_matrix_free(R);
  gsl_matrix_free(Q);
  gsl_matrix_free(Q_total);
  gsl_matrix_free(Q_temp);
  gsl_matrix_free(A);
  gsl_matrix_free(B);
  gsl_matrix_free(temp_holding_matrix);
  return count;
}


bool QRconvergence(gsl_matrix * A)
{
  gsl_matrix * temp1 = gsl_matrix_alloc(A->size1,A->size2);
  gsl_matrix * temp2 = gsl_matrix_calloc(A->size1,A->size2);
  gsl_vector_view diag;

  gsl_matrix_memcpy(temp1, A);
  diag = gsl_matrix_diagonal(A);
  setDiagonal(temp2, &diag.vector);
  gsl_matrix_sub(temp1, temp2);
  absMatrix(temp1);
  if(gsl_matrix_max(temp1) < 1.0E-10 )
    return true;
  else
    return false;
}

void setDiagonal(gsl_vector * diag, gsl_matrix * M)
{
    int i;
    for(i=0;i<diag->size ;i++)
    {
      gsl_matrix_set(M,i,i,gsl_vector_get(diag,i));
    }
}

void absMatrix(gsl_matrix * M)
{
  int i,j;
  double temp;
  for(i=0;i< M->size1 ;i++)
  {
    for(j=0;j<M->size2 ;j++)
    {
      if(gsl_matrix_get(M,i,j) < 0.0  )
      {
        temp =  - gsl_matrix_get(M,i,j);
        gsl_matrix_set(M,i,j,temp);
      }
    }
  }
}


double convergenceTest(gsl_matrix * S,double beta,int j)
{

  double vec_max, vec_min;
  int N_rows = S->size1
  gsl_vector *temp_vec = gsl_vector_alloc(S->size1);
  gsl_vector_const_view S_column = gsl_matrix_const_column(S,j-1);

  gsl_vector_memcpy(temp_vec,&S_column.vector);
  gsl_vector_scale(temp_vec,beta);
  vec_max = gsl_vector_max(temp_vec);
  vec_min = -gsl_vector_min(temp_vec);
  if( vec_max>vec_min )
    return  vec_max;
  else
    return vec_min;
}
