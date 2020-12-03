// predict.cpp

#include "predict.h"

//defualt constructor
Predict::Predict() {

}

//default destructor
Predict::~Predict() {

    // Delete capture object
    destroyAllWindows(); // destroy the all open windows
    capture.release();   // Delete capture object
}

void Predict::predictApp(char key) {

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

        // Manual reset the keyboard
        if (key == ' ')
            backGroundMOG2 = createBackgroundSubtractorMOG2(10000, 200, false);

        //f3_identify_letter
        if (feature_image.size() > 0 && frames++ > SAMPLE_RATE && feature_image[maxIndex].size() >= 5)
        {
            RotatedRect testRect = fitEllipse(feature_image[maxIndex]);
            //fits an ellipse around a set of 2d points. The function calculates the ellipse that fits(in a least-sense) a set of 2D points best of all.
            //it returns the rotated rectangle in which the ellipse is inscribed. the first algorithm - Param(points input 2d point set, stored in std::vector<> or Mat)

            frames = 0;

            double lowestDiff = HUGE_VAL;

            for (int i = 0; i < MAX_LETTERS; i++)
            {
                if (letters[i].size() == 0)
                    continue;

                double difference = distance(letters[i], feature_image[maxIndex]);

                if (difference < lowestDiff)
                {
                    lowestDiff = difference;
                    asl_letter = 'a' + i;
                }
            }

            if (lowestDiff > DIFF_THRESH)
            { // Dust
                asl_letter = 0;
            }

            cout << "The letter is: " << asl_letter << " | difference: " << lowestDiff << endl;
   /*         displayLetter();*/
        }
    }

    // Delete capture object
    destroyAllWindows(); // destroy the all open windows
    capture.release();   // Delete capture object
}

void Predict::displayLetter() {

    int letterCount = 0;                    // number of letters captured since last display
    char lastLetters[NUM_LAST_LETTERS] = { 0 };

    //creates a Mat object filled with zeros
    Mat letter_image = Mat::zeros(200, 200, CV_8UC3);
    char lastExecLetter = 0;                // last letter sent

  

        //cout << "\nThread #5: Display output\n";

        letterCount %= NUM_LAST_LETTERS;         // Show majority of last letters captured
        lastLetters[letterCount++] = asl_letter; // input from f4
        letter_image = Mat::zeros(200, 200, CV_8UC3);

        int counts[MAX_LETTERS + 1] = { 0 };

        for (int i = 0; i < NUM_LAST_LETTERS; i++)
            counts[lastLetters[i] + 1 - 'a']++;

        int maxCount = 0;
        char maxChar = 0;
        for (int i = 0; i < MAX_LETTERS + 1; i++)
        {
            if (counts[i] > maxCount)
            {
                maxCount = counts[i];
                maxChar = i;
            }
        }

        if (maxChar && maxCount >= MIN_FREQ)
        {
            maxChar = maxChar - 1 + 'a';
            char buffer[2 * sizeof(char)];
            sprintf_s(buffer, "%c", maxChar);

            putText(letter_image, buffer, Point(10, 75), FONT_HERSHEY_SIMPLEX, 12, Scalar(255, 255, 255), 1, 1);

            vector<vector<Point>> dummy;

            dummy.push_back(letters[maxChar - 'a']);

            drawContours(letter_image, dummy, 0, Scalar(255, 0, 0), 2, 8, hierarchy, 0, Point(0, 0));
            if (maxChar != lastExecLetter)
            {
                lastExecLetter = maxChar;
            }
        }

        imshow("Letter", letter_image); // output f5--> letter_image
        char q = waitKey(33);

        //********************************//
 /*       cout << "\nDisplay letter exeuted...\n";*/

 }