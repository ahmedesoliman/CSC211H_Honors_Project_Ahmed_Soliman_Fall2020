//train.cpp
#include "train.h"

//default constructor
Train::Train() {

}

//default destructor

Train::~Train() {

    destroyAllWindows(); // destroy the all open windows
    capture.release();   // Delete capture object
}

void Train::trainApp(char key) {

    capture = VideoCapture(0);

    //Creates MOG2 Background Subtractor.
    backGroundMOG2 = createBackgroundSubtractorMOG2(10000, 200, false);

    while (key != KEY_ESC)
    {
        /*    cout << "inside training \n";*/
        if (!capture.isOpened())
        {
            // Error in opening the video input
            cout << "Cannot Open Webcam... " << endl;
            exit(EXIT_FAILURE);
        }

        Mat fgMaskMOG2; // foreground mask foreground mask generated by MOG2 method

        if (!capture.read(frame))
        {
            cout << "Unable to read next frame." << endl;
            cout << "Exiting..." << endl;
            exit(EXIT_FAILURE);
        }

        // Crop Frame to smaller region using the rectangle of interest method
        Rect myROI(200, 200, 200, 200);

        Mat cropFrame = frame(myROI);

        // Update the background model
        backGroundMOG2->apply(cropFrame, fgMaskMOG2, 0);

        // Detect edges using Threshold
        /*adaptiveThreshold(fgMaskMOG2, threshold_output, THRESH, ADAPTIVE_THRESH_MEAN_C, THRESH_BINARY, 3, 0);  -- adabtiveThreshhold can be another method to be used*/
        threshold(fgMaskMOG2, threshold_output, THRESH, 255, THRESH_BINARY);

        // Find contours
        findContours(threshold_output, feature_image, hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE, Point(0, 0));

        // Find largest contour

        Mat drawing1 = Mat::zeros(cropFrame.size(), CV_8UC3); // empty matrix to draw the contours on it - intialize it to Mat::zeros means all black
        double largest_area = 0;

        for (int j = 0; j < feature_image.size(); j++)
        {
            double area = contourArea(feature_image[j], false); // Find the area of the contour
            if (area > largest_area)
            {
                largest_area = area;
                maxIndex = j; // Store the index of largest contour
            }
        }

        // Draw Largest Contours
        Scalar color = Scalar(0, 0, 255);
        drawContours(drawing1, feature_image, maxIndex, Scalar(255, 255, 255), FILLED); // fill white

        // Draw Contours
        Mat contourImg = Mat::zeros(cropFrame.size(), CV_8UC3);
        drawContours(contourImg, feature_image, maxIndex, Scalar(0, 0, 255), 2, 8, hierarchy, 0, Point(0, 0));

        //Reset if too much noise
        /*           Scalar sums = sum(drawing1);
               int s = sums[0] + sums[1] + sums[2] + sums[3];
               if (s >= RESET_THRESH)
               {
                   backGroundMOG2 = createBackgroundSubtractorMOG2(10000, 200, false);
                   continue;
               }*/

        // Show the current frame and the foreground masks
        imshow("Crop Frame", cropFrame);
        imshow("Mask", drawing1);
        imshow("Foregound Mask", fgMaskMOG2);
        imshow("Contour image", contourImg);


        //if (contourImg.rows > 0)
        //    imshow("Contour", contourImg);

        key = waitKey(1);

        if (key >= 'a' && key <= 'z')
        {
            cout << "Wrote letter '" << (char)key << '\'' << endl;

            // save in memory
            letters[key - 'a'] = contours[maxIndex];

            // write to folder
            char buffer[13 * sizeof(char)];
            sprintf_s(buffer, "./train/%c.jpg", (char)key);
            imwrite(buffer, drawing1);
        }

        // Manual reset the keyboard
        if (key == ' ')
            backGroundMOG2 = createBackgroundSubtractorMOG2(10000, 200, false);

    }

    // Delete capture object
    destroyAllWindows(); // destroy the all open windows
    capture.release();   // Delete capture object
}