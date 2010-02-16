/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * Written (W) 2009-2010 Soeren Sonnenburg
 * Copyright (C) 2009 Fraunhofer Institute FIRST and Max-Planck-Society
 * Copyright (C) 2010 Berlin Institute of Technology
 */

#include "features/CombinedDotFeatures.h"
#include "lib/io.h"
#include "lib/Mathematics.h"

using namespace shogun;

CCombinedDotFeatures::CCombinedDotFeatures() : CDotFeatures()
{
	feature_list=new CList<CDotFeatures*>(true);
	update_dim_feature_space_and_num_vec();
}

CCombinedDotFeatures::CCombinedDotFeatures(const CCombinedDotFeatures & orig)
: CDotFeatures(orig), num_vectors(orig.num_vectors),
	num_dimensions(orig.num_dimensions)
{
}

CFeatures* CCombinedDotFeatures::duplicate() const
{
	return new CCombinedDotFeatures(*this);
}

CCombinedDotFeatures::~CCombinedDotFeatures()
{
	delete feature_list;
}

void CCombinedDotFeatures::list_feature_objs()
{
	SG_INFO( "BEGIN COMBINED DOTFEATURES LIST (%d, %d) - ", num_vectors, num_dimensions);
	this->list_feature_obj();

	CListElement<CDotFeatures*> * current = NULL ;
	CDotFeatures* f=get_first_feature_obj(current);

	while (f)
	{
		f->list_feature_obj();
		f=get_next_feature_obj(current);
	}

	SG_INFO( "END COMBINED DOTFEATURES LIST (%d, %d) - ", num_vectors, num_dimensions);
	this->list_feature_obj();
}

void CCombinedDotFeatures::update_dim_feature_space_and_num_vec()
{
	CListElement<CDotFeatures*> * current = NULL ;
	CDotFeatures* f=get_first_feature_obj(current);

	int32_t dim=0;
	int32_t vec=-1;

	while (f)
	{
		dim+= f->get_dim_feature_space();
		if (vec==-1)
			vec=f->get_num_vectors();
		else if (vec != f->get_num_vectors())
		{
			f->list_feature_obj();
			SG_ERROR("Number of vectors (%d) mismatches in above feature obj (%d)\n", vec, f->get_num_vectors());
		}

		f=get_next_feature_obj(current);
	}

	num_dimensions=dim;
	num_vectors=vec;
	SG_DEBUG("vecs=%d, dims=%d\n", num_vectors, num_dimensions);
}

float64_t CCombinedDotFeatures::dot(int32_t vec_idx1, int32_t vec_idx2)
{
	float64_t result=0;

	CListElement<CDotFeatures*> * current = NULL ;
	CDotFeatures* f=get_first_feature_obj(current);

	while (f)
	{
		result += f->dot(vec_idx1, vec_idx2)*CMath::sq(f->get_combined_feature_weight());
		f=get_next_feature_obj(current);
	}

	return result;
}

float64_t CCombinedDotFeatures::dense_dot(int32_t vec_idx1, const float64_t* vec2, int32_t vec2_len)
{
	float64_t result=0;

	CListElement<CDotFeatures*> * current = NULL ;
	CDotFeatures* f=get_first_feature_obj(current);
	uint32_t offs=0;

	while (f)
	{
		int32_t dim = f->get_dim_feature_space();
		result += f->dense_dot(vec_idx1, vec2+offs, dim)*f->get_combined_feature_weight();
		offs += dim;
		f=get_next_feature_obj(current);
	}

	return result;
}

void CCombinedDotFeatures::dense_dot_range(float64_t* output, int32_t start, int32_t stop, float64_t* alphas, float64_t* vec, int32_t dim, float64_t b)
{
	if (stop<=start)
		return;
	ASSERT(dim==num_dimensions);

	CListElement<CDotFeatures*> * current = NULL;
	CDotFeatures* f=get_first_feature_obj(current);
	uint32_t offs=0;
	bool first=true;
	int32_t num=stop-start;
	float64_t* tmp=new float64_t[num];

	while (f)
	{
		int32_t f_dim = f->get_dim_feature_space();
		if (first)
		{
			f->dense_dot_range(output, start, stop, alphas, vec+offs, f_dim, b);
			first=false;
		}
		else
		{
			f->dense_dot_range(tmp, start, stop, alphas, vec+offs, f_dim, b);
			for (int32_t i=0; i<num; i++)
				output[i]+=tmp[i];
		}
		offs += f_dim;
		f=get_next_feature_obj(current);
	}
	delete[] tmp;
}

void CCombinedDotFeatures::dense_dot_range_subset(int32_t* sub_index, int32_t num, float64_t* output, float64_t* alphas, float64_t* vec, int32_t dim, float64_t b)
{
	if (num<=0)
		return;
	ASSERT(dim==num_dimensions);

	CListElement<CDotFeatures*> * current = NULL;
	CDotFeatures* f=get_first_feature_obj(current);
	uint32_t offs=0;
	bool first=true;
	float64_t* tmp=new float64_t[num];

	while (f)
	{
		int32_t f_dim = f->get_dim_feature_space();
		if (first)
		{
			f->dense_dot_range_subset(sub_index, num, output, alphas, vec+offs, f_dim, b);
			first=false;
		}
		else
		{
			f->dense_dot_range_subset(sub_index, num, tmp, alphas, vec+offs, f_dim, b);
			for (int32_t i=0; i<num; i++)
				output[i]+=tmp[i];
		}
		offs += f_dim;
		f=get_next_feature_obj(current);
	}
	delete[] tmp;
}

void CCombinedDotFeatures::add_to_dense_vec(float64_t alpha, int32_t vec_idx1, float64_t* vec2, int32_t vec2_len, bool abs_val)
{
	CListElement<CDotFeatures*> * current = NULL ;
	CDotFeatures* f=get_first_feature_obj(current);
	uint32_t offs=0;

	while (f)
	{
		int32_t dim = f->get_dim_feature_space();
		f->add_to_dense_vec(alpha*f->get_combined_feature_weight(), vec_idx1, vec2+offs, dim, abs_val);
		offs += dim;
		f=get_next_feature_obj(current);
	}
}


int32_t CCombinedDotFeatures::get_nnz_features_for_vector(int32_t num)
{
	CListElement<CDotFeatures*> * current = NULL ;
	CDotFeatures* f=get_first_feature_obj(current);
	int32_t result=0;

	while (f)
	{
		result+=f->get_nnz_features_for_vector(num);
		f=get_next_feature_obj(current);
	}

	return result;
}

void CCombinedDotFeatures::get_subfeature_weights(float64_t** weights, int32_t* num_weights)
{
	*num_weights = get_num_feature_obj();
	ASSERT(*num_weights > 0);

	*weights=new float64_t[*num_weights];
	float64_t* w = *weights;

	CListElement<CDotFeatures*> * current = NULL;	
	CDotFeatures* f = get_first_feature_obj(current);

	while (f)
	{
		*w++=f->get_combined_feature_weight();
		f = get_next_feature_obj(current);
	}
}

void CCombinedDotFeatures::set_subfeature_weights(
	float64_t* weights, int32_t num_weights)
{
	int32_t i=0 ;
	CListElement<CDotFeatures*> * current = NULL ;	
	CDotFeatures* f = get_first_feature_obj(current);

	ASSERT(num_weights==get_num_feature_obj());

	while(f)
	{
		f->set_combined_feature_weight(weights[i]);
		f = get_next_feature_obj(current);
		i++;
	}
}
