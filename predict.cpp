#include "predict.h"


//defualt constructor
Predict::Predict() {


}

// default destructor
Predict::~Predict() {

    destroyAllWindows();
    capture.release();
}

//
void Predict::asl_init()
{
    int numframe = 0;
    Mat frame;

    //************Preload letter images starts*********//
    for (int i = 0; i < MAX_LETTERS; i++)
    {
        char buffer[13 * sizeof(char)];
        sprintf_s(buffer, "images/%c.jpg", (char)('a' + i));
        Mat img1 = imread(buffer, 1);
        if (img1.data)
        {
            Mat img2;
            cvtColor(img1, img2, COLOR_RGB2GRAY);
            Mat threshold_output;

            // Detect edges using Threshold
            //The threshold method returns two outputs. The first is the threshold that was used and the second output is the thresholded image.
            threshold(img2, threshold_output, THRESH, 255, THRESH_BINARY);

            //findcontours() function retrieves contours from the binary image using the openCV algorithm[193].
            //The contours are a useful tool for shape analysisand object and detectionand recognition.
            findContours(threshold_output, contours, hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE, Point(0, 0));

            letters[i] = contours[0];
        }
    }

    //************Preload letter images ends*********//



    //************learn starts**********************//

    backGroundMOG2 = createBackgroundSubtractorMOG2(10000, 200, false);

    //************learn ends  **********************//
}

//this function capture the image
void Predict::f1_captureimage()
{
    Mat frame;
    capture = VideoCapture(0);
    // current frame
    while (1)
    {
        printf("Thread #1: Capture Image\n\r");
        // Create the capture object
        if (!capture.isOpened())
        {
            cout << "Cannot Open Webcam !!!" << endl; // Error in opening the video input
            exit(EXIT_FAILURE);
        }
        // Read the current frame
        if (!capture.read(frame))
        {
            cout << "Unable to read next frame." << endl;
            cout << "Exiting..." << endl;
            exit(EXIT_FAILURE);
        }
        Rect myROI(100, 100, 200, 200); // Crop Frame to smaller region : output --> rgb_image

        rgb_image = frame(myROI);

        imshow("th1_captureimage", rgb_image); // output th1--> rgb_imge

        char q = waitKey(33);

        printf("\nThread #1 exeuted...\n");
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    capture.release();
    std::this_thread::sleep_for(std::chrono::seconds(10));
}

//this function extract the hand
void Predict::f2_extracthand()
{
    while (1)
    {
        /*      Mat binary_image;*/
        printf("Thread #2: Extract hand\n\r");

        if (reset <= 10)
        {
            reset++;
            backGroundMOG2 = createBackgroundSubtractorMOG2(10000, 16, true);
        }

        backGroundMOG2->apply(rgb_image, binary_image, 0);

        imshow("Binary Image", binary_image);
        printf("\nThread #2 exeuted...\n");
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    std::this_thread::sleep_for(std::chrono::seconds(10));
}

//this function extracts the feature
void Predict::f3_extractfeature()
{
    while (true)
    {

        Mat threshold_output; // Generate the tresholdoutput

        printf("Thread #3: Extract Feature\n\r");

        threshold(binary_image, threshold_output, THRESH, 255, THRESH_BINARY); // Detect edges using Threshold

        findContours(threshold_output, feature_image, hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE, Point(0, 0)); // Find contours

        imshow("feature", feature_image);

        drawing = Mat::zeros(rgb_image.size(), CV_8UC3); // Find largest contour

        double largest_area = 0;

        for (int j = 0; j < feature_image.size(); j++)
        {
            double area = contourArea(feature_image[j], false); // Find the area of contour
            if (area > largest_area)
            {
                largest_area = area;
                maxIndex = j; // Store the index of largest contour
            }
        }

        //printf("%d", maxIndex);  // Draw Largest Contours
        Scalar color = Scalar(0, 0, 255);
        //To draw the contours, cv::drawContours function is used
        drawContours(drawing, feature_image, maxIndex, Scalar(255, 255, 255), FILLED); // filled white

        // Draw Contours
        Mat contourImg = Mat::zeros(rgb_image.size(), CV_8UC3);
        drawContours(contourImg, feature_image, maxIndex, Scalar(0, 0, 255), 2, 8, hierarchy, 0, Point(0, 0));

        // Reset if too much noise
        Scalar sums = sum(drawing);
        int s = sums[0] + sums[1] + sums[2] + sums[3];
        if (s >= RESET_THRESH)
        {
            reset = 10;
        }

        imshow("Foreground", drawing);
        if (contourImg.rows > 0)
            imshow("th3_extractfeature", contourImg);
        char q = waitKey(33);


        //****
        printf("\nThread #3 executed...\n");
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

    }
    std::this_thread::sleep_for(std::chrono::seconds(10));
}

//this fucntion identifies the letter
void Predict::f4_identifyletter()
{
    while (true)
    {
        printf("Thread #4: Identify letter\n\r");

        // Compare to reference images
        if (feature_image.size() > 0 && frames++ > SAMPLE_RATE && feature_image[maxIndex].size() >= 5)
        {
            RotatedRect testRect = fitEllipse(feature_image[maxIndex]);
            frames = 0;
            double lowestDiff = HUGE_VAL;
            for (int i = 0; i < MAX_LETTERS; i++)
            {
                if (letters[i].size() == 0)
                    continue;

                double diff = distance(letters[i], feature_image[maxIndex]);

                if (diff < lowestDiff)
                {
                    lowestDiff = diff;
                    asl_letter = 'a' + i;
                }
            }
            if (lowestDiff > DIFF_THRESH)
            { // Dust
                asl_letter = 0;
            }
            cout << asl_letter << " | diff: " << lowestDiff << endl;
            printf("| diff: %f \n\r", lowestDiff);
        }
        //************
        printf("\nThread #4 exeuted... \n");
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    std::this_thread::sleep_for(std::chrono::seconds(10));
}

//this fucntion displays the letter
void Predict::f5_displayletter()
{
    int letterCount = 0; // number of letters captured since last display
    char lastLetters[NUM_LAST_LETTERS] = { 0 };
    
    //creates a Mat object filled with zeros
    Mat letter_image = Mat::zeros(200, 200, CV_8UC3);
    char lastExecLetter = 0; // last letter sent to doSystemCalls()

    while (1)
    {
        printf("Thread #5: Display output\n\r");

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
                //doSystemCalls(maxChar);
            }
        }
        imshow("Letter", letter_image); // output th5--> letter_image
        char q = waitKey(33);

        printf("\nThread #5 exeuted...\n");
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    this_thread::sleep_for(chrono::seconds(10));
}

// this function returns max distance between two vector points a and b 
int Predict::distance_2(vector<Point> a, vector<Point> b)
{
    int maxDistAB = 0;
    for (size_t i = 0; i < a.size(); i++)
    {
        int minB = 1000000;
        for (size_t j = 0; j < b.size(); j++)
        {
            int dx = (a[i].x - b[j].x);
            int dy = (a[i].y - b[j].y);
            int tmpDist = dx * dx + dy * dy;

            if (tmpDist < minB)
            {
                minB = tmpDist;
            }
            if (tmpDist == 0)
            {
                break; // can't get better than equal.
            }
        }
        maxDistAB += minB;
    }
    return maxDistAB;
}

//
double Predict::distance(vector<Point> a, vector<Point> b)
{
    int maxDistAB = distance_2(a, b);
    int maxDistBA = distance_2(b, a);
    int maxDist = max(maxDistAB, maxDistBA);

    return sqrt((double)maxDist);
}

////
//void Predict::threads_run() {
//    std::thread f1(&Predict::f1_captureimage);
//    std::thread f2(&Predict::f2_extracthand);
//    std::thread f3(&Predict::f3_extractfeature);
//    std::thread f4(&Predict::f4_identifyletter);
//    std::thread f5(&Predict::f5_displayletter);
//    f1.join();
//    f2.join();
//    f3.join();
//    f4.join();
//    f5.join();
//}

//this function runs the application
void Predict::run(char key) {

        capture = VideoCapture(0);

        //Creates MOG2 Background Subtractor.
        backGroundMOG2 = createBackgroundSubtractorMOG2(10000, 200, false);

        while (key != KEY_ESC)
        {
            printf("inside training \n\r ");
            if (!capture.isOpened())
            {
                // Error in opening the video input
                cout << "Cannot Open Webcam... " << endl;
                exit(EXIT_FAILURE);
            }

            Mat frame;                                      // current frame
            Mat fgMaskMOG2;                                 // foreground mask foreground mask generated by MOG2 method
            Mat threshold_output;                           // Generate Convex Hull
            vector<vector<Point>> contours;                 // local vector to store the points of the contours

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
            findContours(threshold_output, contours, hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE, Point(0, 0));

            // Find largest contour
            Mat drawing1 = Mat::zeros(cropFrame.size(), CV_8UC3); // empty matrix to draw the contours on it - intialize it to Mat::zeros means all black
            double largest_area = 0;
            int maxIndex = 0;
            
            for (int j = 0; j < contours.size(); j++)
            {
                double area = contourArea(contours[j], false); // Find the area of the contour
                if (area > largest_area)
                {
                    largest_area = area;
                    maxIndex = j; // Store the index of largest contour
                }
            }

            // Draw Largest Contours
            Scalar color = Scalar(0, 0, 255);
            drawContours(drawing1, contours, maxIndex, Scalar(255, 255, 255), FILLED); // fill white
                                                                                       
            // Draw Contours
            Mat contourImg = Mat::zeros(cropFrame.size(), CV_8UC3);
            drawContours(contourImg, contours, maxIndex, Scalar(0, 0, 255), 2, 8, hierarchy, 0, Point(0, 0));

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
      /*      imshow("Foregound Mask", fgMaskMOG2);*/
            imshow("Contour image", contourImg);

            //if (contourImg.rows > 0)
            //    imshow("Contour", contourImg);
            //char ch = cvWaitKey(33);

            key = waitKey(1);

            if (key >= 'a' && key <= 'z')
            {
                cout << "Wrote letter '" << (char)key << '\'' << endl;

                // save in memory
                letters[key - 'a'] = contours[maxIndex];

                // write to file
                char buffer[13 * sizeof(char)];
                sprintf_s(buffer, "images/%c.jpg", (char)key);
                imwrite(buffer, drawing1);
            }

            // Manual reset the keyboard
            if (key == ' ')
                backGroundMOG2 = createBackgroundSubtractorMOG2(10000, 200, false);
            // Delete capture object
        }

        destroyAllWindows(); // destroy the all open windows
        capture.release();  // Delete capture object
 }

void  Predict::train() {

    String folder1path = "./data/*.jpg";
    String folder2 = "./images/*."
    vector<String> filenames;

    cv::glob(folderpath, filenames);



    for (size_t i = 0; i < filenames.size(); i++)
    {
        Mat loaded_img = imread(filenames[i], IMREAD_GRAYSCALE);
        substract(loaded_img, )
    }

 }