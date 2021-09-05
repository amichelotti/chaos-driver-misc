#include "rootGaussianImage2dFit.h"
#include <TF2.h>
#include <TH2F.h>
#include <chaos/common/global.h>
namespace root
{
    namespace image
    {
        inline static double beamFunc(double *x, double *p)
        {
            double amplitude = p[0];
            double mean_x = p[1];
            double sigma_x = p[2];
            double mean_y = p[3];
            double sigma_y = p[4];
            double rho = p[5];
            double u = (x[0] - mean_x) / sigma_x;
            double v = (x[1] - mean_y) / sigma_y;
            double c = 1 - rho * rho;
            double result = amplitude * exp(-(u * u - 2 * rho * u * v + v * v) / (2 * c));

            //(1 / (2 * TMath::Pi() * sigma_x * sigma_y * sqrt(c)))

            return result;
        }
        using namespace cv;

        int rootGaussianImage2dFit(cv::Mat &image, int thrshold,int fit_level, double &Amplitude, double &X_m, double &Y_m, double &S_x, double &S_y, double &rho){
            TH2F *h;
            TF2 *g2d;
            cv::Mat thr,gray;
            int res=0;
            try
            {
                uint32_t width = image.cols;
                uint32_t height = image.rows;
                if (image.channels() >= 3)
                {
                    cv::Mat imagec;
                    cvtColor(image, imagec, COLOR_BGR2GRAY);
                    threshold(imagec, thr, thrshold, 255, THRESH_TOZERO);
                }
                else
                {
                    threshold(image, thr, thrshold, 255, THRESH_BINARY);
                }

                h = new TH2F("slm", " ", thr.cols, 0, thr.cols, thr.rows, 0,
                             thr.rows);
                h->SetStats(0);
                g2d = new TF2("g2d", beamFunc, 0., thr.cols, 0., thr.rows, 6);
                // g2d->Draw("SAME");
                g2d->SetParNames("Amplitude", "X_m", "S_x", "Y_m", "S_y", "rho");

                int xPixels = thr.cols;
                int yPixels = thr.rows;
                for (int row = 0; row < xPixels; ++row)
                {
                    for (int col = 0; col < yPixels; ++col)
                    {
                        // float greyv = (255.0*1.0-gray.at<uchar>(row,col))/256.0;
                        uchar v = thr.at<uchar>(col, row);
                        float greyv = 0;
                        if (v >= thrshold)
                        {
                            greyv = (thr.at<uchar>(col, row)) / 256.0;
                        }
                        h->SetBinContent(row, col, greyv);
                        //  h->SetBinError(row+1,yPixels-col,TMath::Sqrt(grey));
                    }
                }
                Amplitude = h->GetMaximum();
                X_m = h->GetMean(1);
                S_x = h->GetRMS(1);
                Y_m = h->GetMean(2);
                S_y = h->GetRMS(2);
                rho = h->GetCorrelationFactor();
                if(fit_level>0){
                    double par[6] = {Amplitude, X_m, S_x, Y_m, S_y, rho};
                    g2d->SetParameters(par);

                    LDBG_ << "Start Fitting params: A:" << par[0] << " Mx:" << par[1]
                        << " Sx:" << par[2] << " My:" << par[3] << " Sy:" << par[4]
                        << " rho:" << par[5];

                    res =h->Fit("g2d", "N", "");
                    Amplitude = g2d->GetParameter(0);
                    X_m = g2d->GetParameter(1);
                    S_x = g2d->GetParameter(2);
                    Y_m = g2d->GetParameter(3);
                    S_y = g2d->GetParameter(4);
                    rho = g2d->GetParameter(5);
                }

            LDBG_ << "End Fitting C:" << X_m << "," << Y_m << " S:" << S_x << ","
                  << S_y << " angle:" << rho;

            delete h;
            delete g2d;
            return res;
        }
        catch (std::runtime_error &e)
        {
            std::cout << "decode exception:" << e.what();
            return -1;
        }
        catch (cv::Exception &e)
        {
            std::cout << "cv:decode exception:" << e.what();
            return -2;
        }
        catch (...)
        {
            return -3;
        }
        return 0;
    }
}
}

