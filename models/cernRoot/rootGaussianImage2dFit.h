#ifndef __ROOT_GAUSSIAN_2DFIT__
#define __ROOT_GAUSSIAN_2DFIT__
#ifdef OPENCV
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc.hpp>

namespace root{
    namespace image{
        /**
         * @brief 
         * 
         * @param image input opencv image
         * @param threshold threshold to apply (0,255)
         * @param Amplitude return Amplitude
         * @param X_m return X center
         * @param Y_m return Y center
         * @param S_x return Sigma X
         * @param S_y return Sigma Y
         * @param rho return angle, correlation factor
         * @return 0 if ok, otherwise if error 
         */
        
        int rootGaussianImage2dFit(cv::Mat &image,int threshold,int fit_level,double &Amplitude,double& X_m,double& Y_m,double&S_x,double&S_y,double&rho);
    }
}
#endif

#endif