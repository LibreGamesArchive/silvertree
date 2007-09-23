#ifdef PROTOTYPE_FRUSTUM_CULLING_ENABLED

#include "camera.hpp"
#include "frustum.hpp"

namespace hex {

GLfloat frustum::comb_[16];

void frustum::initialize() 
{
	GLfloat proj[16], mod[16];
	glGetFloatv(GL_PROJECTION_MATRIX, proj);
	glGetFloatv(GL_MODELVIEW_MATRIX, mod);

	for(int i=0;i<4;++i) {
		for(int j=0;j<4;++j) {
			comb_[i*4 + j] = 0.0;
			for(int k=0;k<4;++k) {
				comb_[i*4 + j] += mod[i*4+k] * proj[k*4+j];
			}
		}
	}
}

void frustum::set_volume_clip_space(GLfloat xmin, GLfloat xmax,
				    GLfloat ymin, GLfloat ymax,
				    GLfloat zmin, GLfloat zmax)
{
	for(int i=0;i<4;++i) {
		planes_[0*4 + i] = -xmin*comb_[i*4 + 3] + comb_[i*4 + 0];
		planes_[1*4 + i] =  xmax*comb_[i*4 + 3] - comb_[i*4 + 0];
		planes_[2*4 + i] = -ymin*comb_[i*4 + 3] + comb_[i*4 + 1];
		planes_[3*4 + i] =  ymax*comb_[i*4 + 3] - comb_[i*4 + 1];
		planes_[4*4 + i] = -zmin*comb_[i*4 + 3] + comb_[i*4 + 2];
		planes_[5*4 + i] =  zmax*comb_[i*4 + 3] - comb_[i*4 + 2];
	}

	for(int i=0;i<6;++i) {
		GLfloat d = planes_[i*4+0]*planes_[i*4+0] + planes_[i*4+1]*planes_[i*4+1] + planes_[i*4+2]+planes_[i*4+2];
		d = sqrt(d);
		if(d > 0) {
			planes_[i*4+0] /= d;
			planes_[i*4+1] /= d;
			planes_[i*4+2] /= d;
			planes_[i*4+3] /= d;
		}
	}
}

void frustum::set_volume_world_space(GLfloat radius)
{
	camera *cam = camera::current_camera();
	
	GLfloat wcpos[4], cpos[4], dpos[4], wdpos[4];
	cpos[0] = -cam->get_pan_x() - radius;
	cpos[1] = -cam->get_pan_y() - radius;
	cpos[2] = -cam->get_pan_z() - radius;
	cpos[3] = 1;

	dpos[0] = -cam->get_pan_x() + radius;
	dpos[1] = -cam->get_pan_y() + radius;
	dpos[2] = -cam->get_pan_z() + radius;
	dpos[3] = 1;

	for(int i=0;i<4;++i) {
		wcpos[i] = cpos[0]*comb_[0*4+i] + cpos[1]*comb_[1*4+i] + cpos[2]*comb_[2*4+i] + cpos[3]*comb_[3*4+i];
		wdpos[i] = dpos[0]*comb_[0*4+i] + dpos[1]*comb_[1*4+i] + dpos[2]*comb_[2*4+i] + dpos[3]*comb_[3*4+i];
	}
	for(int i=0;i<4;++i) {
		wcpos[i] /= wcpos[3];
		wdpos[i] /= wdpos[3];
	}

	GLfloat xmax, xmin, ymax, ymin;

	if(wcpos[0] < wdpos[0]) {
		xmin = wcpos[0];
		xmax = wdpos[0];
	} else {
		xmin = wdpos[0];
		xmax = wcpos[0];
	}

	if(wcpos[1] < wdpos[1]) {
		ymin = wcpos[1];
		ymax = wdpos[1];
	} else {
		ymin = wdpos[1];
		ymax = wcpos[1];
	}

	set_volume_clip_space(xmin, xmax, ymin, ymax, -1, wdpos[2] * (wdpos[2] < 0 ? 1.05 : 0.95));
}

}

#endif
