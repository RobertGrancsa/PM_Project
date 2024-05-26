import express from "express";
import db from "../db/database.mjs";
import { ObjectId } from "mongodb";

const router = express.Router();

router.get("/speed", async (req, res) => {
  let collection = await db.collection("data");
  let results = await collection.find({})
    .limit(100)
    .toArray();

  res.send(results).status(200);
});

router.get("/camera", async (req, res) => {
  let collection = await db.collection("data");
  let results = await collection.find({})
    .limit(50)
    .toArray();

  res.send(results).status(200);
});

router.post("/log-data", async (req, res) => {
  console.log(req.body);

  if (!req.body) return res.status(403);
  let collection = await db.collection("data");
  let newDocument = req.body;
  newDocument.date = new Date();
  let result = await collection.insertOne(newDocument);

  // return checkForStreetSigns(req, res);
  res.send(result).status(204);
})

const checkForStreetSigns = async (req, res) => {
  const { base64Image } = req.body;

    if (!base64Image) {
      return res.status(400).json({ error: 'base64Image is required' }); 
    }

    try {
      const apiKey = process.env.GOOGLE_CLOUD_API;  // Replace with your Google Cloud API key
      const url = `https://vision.googleapis.com/v1/images:annotate?key=${apiKey}`;

      const requestBody = {
        requests: [
          {
            image: {
              content: base64Image
            },
            features: [
              {
                type: 'LABEL_DETECTION'
              }
            ]
          }
        ]
      };

      const response = await axios.post(url, requestBody);

      const annotations = response.data.responses[0].labelAnnotations;

      console.log(response.data)
      const streetSignAnnotations = annotations.filter(annotation => annotation.description.toLowerCase().includes('street sign'));

      res.json({ streetSigns: streetSignAnnotations });
    } catch (error) {
        console.error('Error:', error.response ? error.response.data : error.message);
        res.status(500).json({ error: 'Failed to annotate image' });
    }
}

export default router;